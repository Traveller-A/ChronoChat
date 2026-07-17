<template>
  <div class="group-chat" v-loading="loading">
    <!-- 顶部导航栏 -->
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <div class="gname-wrap">
        <span class="gname">{{ group.name || '群聊' }}</span>
        <el-tag :type="statusTagType" size="small" effect="dark" class="status-tag">{{ statusText }}</el-tag>
      </div>

      <div class="top-right">
        <div class="mode-toggle">
          <span :class="{ active: chatMode === 'mention' }" @click="switchMode('mention')">
            <el-icon><ChatDotRound /></el-icon> @提及
          </span>
          <span :class="{ active: chatMode === 'auto' }" @click="switchMode('auto')">
            <el-icon><Connection /></el-icon> 自动
          </span>
        </div>
        <el-button :icon="Setting" circle size="small" class="manage-btn" @click="goManage" title="群聊管理" />
      </div>
    </div>

    <!-- 自动模式横幅 -->
    <div v-if="chatMode === 'auto'" class="auto-banner">
      <el-icon><InfoFilled /></el-icon>
      <span>自动对话模式 · 管理员智能体调度角色发言，你可随时插话或 @管理员 设置规则</span>
    </div>

    <!-- 消息区域 -->
    <div class="messages" ref="mc">
      <div v-if="loading" class="loading-msg">
        <span class="signal-pulse"></span> 正在接通信号…
      </div>
      <div v-if="!loading && messages.length === 0" class="empty-msg">
        <i class="empty-glyph">✦</i>
        <p>发送第一条消息，开始群聊吧</p>
      </div>

      <template v-for="(m, i) in messages" :key="i">
        <!-- 系统消息（管理员） -->
        <div v-if="m.role === 'system'" class="system-msg">
          <span class="system-text">{{ m.content }}</span>
        </div>
        <!-- 聊天气泡 -->
        <div v-else :class="['msg-row', m.role === 'user' ? 'msg-user' : 'msg-char']">
          <el-avatar v-if="m.role === 'char'" :size="38" :src="getMemberAvatar(m.senderId)" class="msg-avatar">
            {{ m.sender?.charAt(0) }}
          </el-avatar>
          <div class="msg-bubble tx-line" :class="m.role">
            <div v-if="m.role === 'char'" class="msg-sender">{{ m.sender }}</div>
            <div class="msg-text" v-html="renderContent(m.content)"></div>
            <div class="msg-time">{{ m.time }}</div>
          </div>
          <el-avatar v-if="m.role === 'user'" :size="38" icon="UserFilled" class="msg-avatar user-av" />
        </div>
      </template>

      <!-- 等待回复 -->
      <div v-if="waiting" class="msg-row msg-char">
        <el-avatar :size="38" icon="ChatDotRound" class="msg-avatar typing-av" />
        <div class="msg-bubble char tx-line typing">
          <span class="typing-dots"><span></span><span></span><span></span></span>
        </div>
      </div>
    </div>

    <!-- 底部输入 -->
    <div class="input-bar">
      <div class="input-wrapper">
        <div v-if="showMentionList" class="mention-list">
          <div v-for="m in filteredMembers" :key="m.id" class="mention-item" @click="selectMention(m)">
            <el-avatar :size="28" :src="getMemberAvatar(m.id)">{{ m.name?.charAt(0) }}</el-avatar>
            <span>{{ m.name }}</span>
          </div>
          <div v-if="filteredMembers.length === 0" class="mention-empty">无匹配成员</div>
        </div>
        <el-input ref="inputRef" v-model="text" placeholder="输入消息，@成员名可提及角色…" @keyup.enter="send" @input="onInput" :disabled="waiting" size="large" />
      </div>
      <el-button type="primary" :icon="Promotion" size="large" @click="send" :disabled="waiting || !text.trim()" class="send-btn" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onBeforeUnmount, nextTick, computed } from 'vue'
import { useRouter, useRoute, onBeforeRouteLeave } from 'vue-router'
import { ArrowLeft, Promotion, ChatDotRound, Connection, InfoFilled, Setting } from '@element-plus/icons-vue'
import { getGroup, getGroupHistory, sendGroupMessage, getAvatarUrl, autoStep, setGroupMode, getConfig } from '@/api'
import { ElMessage, ElMessageBox, ElNotification } from 'element-plus'

const router = useRouter()
const route = useRoute()
const gid = route.params.id

const group = ref({})
const members = ref([])
const messages = ref([])
const text = ref('')
const waiting = ref(false)
const loading = ref(true)
const mc = ref(null)
const inputRef = ref(null)

const chatMode = ref('mention')
const showMentionList = ref(false)
const mentionFilter = ref('')
const userName = ref('旅人')  // default
let autoTimer = null
let autoPaused = false

const filteredMembers = computed(() => {
  if (!mentionFilter.value) return members.value
  const kw = mentionFilter.value.toLowerCase()
  return members.value.filter(m => m.name.toLowerCase().includes(kw))
})

const statusText = computed(() =>
  group.value.status === 'open' ? '进行中' : group.value.status === 'paused' ? '已暂停' : '已关闭')
const statusTagType = computed(() =>
  group.value.status === 'open' ? 'success' : group.value.status === 'paused' ? 'warning' : 'info')

function getMemberAvatar(senderId) {
  if (!senderId) return ''
  return getAvatarUrl(senderId)
}

function switchMode(mode) {
  if (mode === chatMode.value) return
  chatMode.value = mode
  // Persist mode change to backend
  setGroupMode(gid, mode).catch(() => {})
  if (mode === 'auto') {
    ElMessage.success('已切换到自动对话模式')
    autoPaused = false
    switchedToMention = false
    resetInactivityTimer()
    startAutoLoop()
  } else {
    stopAutoLoop()
    clearInactivityTimer()
    ElMessage.success('已切换到 @提及模式')
  }
}

// ---- Auto dialogue loop ----
let autoIdleTimer = null

// ---- Inactivity / Leave detection ----
const INACTIVITY_TIMEOUT = 5 * 60 * 1000 // 5 minutes
let inactivityTimer = null
let switchedToMention = false  // prevent duplicate switch calls

function resetInactivityTimer() {
  if (inactivityTimer) clearTimeout(inactivityTimer)
  if (chatMode.value !== 'auto' || switchedToMention) return
  inactivityTimer = setTimeout(autoSwitchToMention, INACTIVITY_TIMEOUT)
}

function clearInactivityTimer() {
  if (inactivityTimer) { clearTimeout(inactivityTimer); inactivityTimer = null }
}

async function autoSwitchToMention() {
  if (switchedToMention || chatMode.value !== 'auto') return
  switchedToMention = true
  chatMode.value = 'mention'
  stopAutoLoop()
  clearInactivityTimer()
  try {
    await setGroupMode(gid, 'mention')
    ElNotification({
      title: '群聊模式已切换',
      message: '长时间未活动，已自动切换为 @提及模式',
      type: 'info',
      duration: 5000
    })
  } catch (e) { /* ignore */ }
}

// Route leave guard
onBeforeRouteLeave(async (_to, _from, next) => {
  if (chatMode.value === 'auto' && !switchedToMention) {
    try {
      await ElMessageBox.confirm(
        '当前为自动对话模式，离开后角色可能继续发言。是否切换为 @提及模式？',
        '离开群聊',
        { confirmButtonText: '切换并离开', cancelButtonText: '保持自动模式', type: 'warning' }
      )
      await autoSwitchToMention()
    } catch (e) { /* user cancelled, leave as-is */ }
  }
  stopAutoLoop()
  clearInactivityTimer()
  next()
})

// Tab close detection
function handleBeforeUnload(e) {
  if (chatMode.value === 'auto' && !switchedToMention) {
    // Try to switch mode via sendBeacon (fire-and-forget)
    const url = `/api/groups/${encodeURIComponent(gid)}/mode`
    const body = JSON.stringify({ mode: 'mention' })
    navigator.sendBeacon(url, new Blob([body], { type: 'application/json' }))
    // Show browser confirmation
    e.preventDefault()
    e.returnValue = ''
  }
}

// Visibility change: when hidden + auto mode, start inactivity countdown
function handleVisibilityChange() {
  if (document.hidden && chatMode.value === 'auto') {
    resetInactivityTimer()
  } else if (!document.hidden) {
    clearInactivityTimer()
  }
}

function startAutoLoop() {
  stopAutoLoop()
  if (chatMode.value !== 'auto') return
  // Start with a 3s delay to give user time to read
  autoTimer = setTimeout(runAutoStep, 2000)
}

function stopAutoLoop() {
  if (autoTimer) { clearTimeout(autoTimer); autoTimer = null }
  if (autoIdleTimer) { clearTimeout(autoIdleTimer); autoIdleTimer = null }
}

function resetIdleTimer() {
  // Reset the idle timer: user just interacted, so pause auto chain for a while
  if (autoIdleTimer) { clearTimeout(autoIdleTimer); autoIdleTimer = null }
  autoIdleTimer = setTimeout(() => {
    if (chatMode.value === 'auto' && !autoPaused) {
      startAutoLoop()
    }
  }, 3000) // 3s idle before characters continue among themselves
}

async function runAutoStep() {
  if (chatMode.value !== 'auto' || autoPaused) { stopAutoLoop(); return }
  try {
    const res = await autoStep(gid)
    if (res.code === 0) {
      const d = res.data
      if (d.action === 'speak' && d.reply) {
        const now = nowTime()
        messages.value.push({
          role: 'char',
          sender: d.character || 'AI',
          senderId: d.character_id || '',
          content: d.reply,
          time: now
        })
        await scrollBottom()
        // After one character speaks, stop chain. Let idle timer continue if user is silent.
        stopAutoLoop()
        resetIdleTimer()
      } else if (d.action === 'wait') {
        stopAutoLoop()
        resetIdleTimer()
      } else {
        // pause or other - stop completely
        if (d.reason) {
          messages.value.push({ role: 'system', content: '[对话暂停] ' + d.reason, time: nowTime() })
        }
        stopAutoLoop()
      }
    } else {
      stopAutoLoop()
    }
  } catch (e) {
    // Retry after delay
    autoTimer = setTimeout(runAutoStep, 5000)
  }
}

// ---- @mention ----
function onInput(val) {
  const cursorPos = inputRef.value?.$el?.querySelector('input')?.selectionStart
  if (cursorPos === undefined) { showMentionList.value = false; return }
  const beforeCursor = val.slice(0, cursorPos)
  const atMatch = beforeCursor.match(/@([\w\u4e00-\u9fff]*)$/)
  if (atMatch) {
    mentionFilter.value = atMatch[1]
    showMentionList.value = true
  } else {
    showMentionList.value = false
  }
}

function selectMention(member) {
  const cursorPos = inputRef.value?.$el?.querySelector('input')?.selectionStart || 0
  const beforeCursor = text.value.slice(0, cursorPos)
  const afterCursor = text.value.slice(cursorPos)
  const atIndex = beforeCursor.lastIndexOf('@')
  const newText = beforeCursor.slice(0, atIndex) + '@' + member.name + ' ' + afterCursor
  text.value = newText
  showMentionList.value = false
  nextTick(() => {
    const inp = inputRef.value?.$el?.querySelector('input')
    if (inp) {
      const newPos = atIndex + member.name.length + 2
      inp.setSelectionRange(newPos, newPos)
      inp.focus()
    }
  })
}

function renderContent(content) {
  if (!content) return ''
  // Replace {{user}} placeholder with user name
  let text = content.replace(/\{\{user\}\}/g, userName.value)
  // Highlight @mentions
  return text.replace(/@([\w\u4e00-\u9fff]+)/g, '<span class="mention">@$1</span>')
}

// ---- Send ----
async function send() {
  const msg = text.value.trim()
  if (!msg || waiting.value) return
  showMentionList.value = false
  text.value = ''
  const now = nowTime()
  messages.value.push({ role: 'user', sender: '{{user}}', senderId: '', content: msg, time: now })

  // Reset inactivity timer on every user message
  resetInactivityTimer()

  // In auto mode, pause auto-loop while waiting for manual response
  if (chatMode.value === 'auto') {
    autoPaused = true
    stopAutoLoop()
  }

  waiting.value = true
  await scrollBottom()
  let hadReply = false
  try {
    const res = await sendGroupMessage(gid, msg, '')
    if (res.code === 0 && res.data.reply) {
      hadReply = true
      messages.value.push({
        role: 'char',
        sender: res.data.responder_name || 'AI',
        senderId: res.data.responder_id || '',
        content: res.data.reply,
        time: nowTime()
      })
      await scrollBottom()
    }
  } catch (e) { ElMessage.error('Send failed') }
  finally {
    waiting.value = false
    await scrollBottom()
    // In auto mode, restart idle timer unless a character already replied via @mention
    if (chatMode.value === 'auto') {
      autoPaused = false
      if (!hadReply) {
        resetIdleTimer()
      }
    }
  }
}

function nowTime() {
  const d = new Date()
  return d.getHours().toString().padStart(2, '0') + ':' + d.getMinutes().toString().padStart(2, '0')
}

async function scrollBottom() {
  await nextTick()
  if (mc.value) mc.value.scrollTop = mc.value.scrollHeight
}

function goBack() {
  stopAutoLoop()
  if (autoIdleTimer) clearTimeout(autoIdleTimer)
  router.push('/groupchats')
}
function goManage() { router.push(`/groupchats/${gid}/manage`) }

onMounted(async () => {
  // Register leave detection events
  window.addEventListener('beforeunload', handleBeforeUnload)
  document.addEventListener('visibilitychange', handleVisibilityChange)

  try {
    const [gr, hr, cfg] = await Promise.all([getGroup(gid), getGroupHistory(gid), getConfig()])
    if (cfg.code === 0 && cfg.data && cfg.data.user_name) {
      userName.value = cfg.data.user_name
    }
    if (gr.code === 0) {
      group.value = gr.data
      members.value = gr.data.member_details || []
      const savedMode = gr.data.mode
      if (savedMode === 'auto' || savedMode === 'mention') {
        chatMode.value = savedMode
      }
    }
    if (hr.code === 0 && hr.data.history) {
      hr.data.history.split('\n').filter(Boolean).forEach(line => {
        const m = line.match(/^\[(.+?)\]:\s*(.+)/)
        if (m) {
          const senderName = m[1]
          const content = m[2]
          const isAdmin = (senderName === 'Admin')
          if (isAdmin) {
            messages.value.push({ role: 'system', sender: 'Admin', content, time: '' })
            return
          }
          const isUser = (senderName === '{{user}}' || senderName === 'Me' || senderName === userName.value)
          const member = members.value.find(mb => mb.name === senderName)
          messages.value.push({
            role: isUser ? 'user' : 'char',
            sender: isUser ? userName.value : senderName,
            senderId: member ? member.id : '',
            content,
            time: ''
          })
        }
      })
    }
  } catch (e) { ElMessage.error('Failed to load') }
  finally {
    loading.value = false
    await scrollBottom()
    // Start auto loop if mode is auto
    if (chatMode.value === 'auto') {
      startAutoLoop()
    }
  }
})

onBeforeUnmount(() => {
  stopAutoLoop()
  if (autoIdleTimer) clearTimeout(autoIdleTimer)
  clearInactivityTimer()
  window.removeEventListener('beforeunload', handleBeforeUnload)
  document.removeEventListener('visibilitychange', handleVisibilityChange)
})
</script>

<style scoped>
.group-chat {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: linear-gradient(180deg, rgba(26, 37, 64, 0.88), rgba(20, 29, 51, 0.94));
}

/* ---------- Top bar ---------- */
.top-bar {
  display: flex; align-items: center; gap: 14px;
  padding: 12px 20px;
  background: rgba(11, 16, 30, 0.72);
  backdrop-filter: blur(14px);
  -webkit-backdrop-filter: blur(14px);
  border-bottom: 1px solid var(--ink-500);
  flex-shrink: 0;
}
.back-btn {
  flex: none;
  background: var(--ink-700) !important;
  border-color: var(--ink-500) !important;
  color: var(--star-soft) !important;
}
.back-btn:hover { border-color: var(--gold-dim) !important; color: var(--gold) !important; }

.gname-wrap { flex: 1; display: flex; align-items: center; gap: 12px; min-width: 0; }
.gname {
  font-family: var(--font-serif);
  font-size: 17px; font-weight: 600;
  color: var(--star);
  letter-spacing: 0.02em;
  white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
}
.status-tag { flex: none; }

.top-right { display: flex; align-items: center; gap: 10px; flex: none; }
.manage-btn {
  background: var(--ink-700) !important;
  border-color: var(--ink-500) !important;
  color: var(--star-soft) !important;
}
.manage-btn:hover { border-color: var(--gold-dim) !important; color: var(--gold) !important; }

.mode-toggle {
  display: flex;
  background: var(--ink-800);
  border: 1px solid var(--ink-500);
  border-radius: 20px;
  padding: 3px;
}
.mode-toggle span {
  display: flex; align-items: center; gap: 5px;
  padding: 5px 14px;
  border-radius: 16px;
  cursor: pointer;
  font-size: 13px;
  color: var(--star-dim);
  transition: all 0.2s ease;
  user-select: none;
}
.mode-toggle span:hover { color: var(--star-soft); }
.mode-toggle span.active {
  background: linear-gradient(135deg, var(--gold), var(--gold-bright));
  color: #1a1408;
  font-weight: 600;
  box-shadow: 0 2px 10px rgba(230, 181, 102, 0.3);
}

/* ---------- Auto banner ---------- */
.auto-banner {
  display: flex; align-items: center; gap: 10px;
  padding: 9px 20px;
  background: linear-gradient(90deg, rgba(94, 179, 196, 0.12), rgba(230, 181, 102, 0.08));
  border-bottom: 1px solid rgba(94, 179, 196, 0.25);
  color: var(--cyan);
  font-size: 13px;
  letter-spacing: 0.02em;
  flex-shrink: 0;
}
.auto-banner .el-icon { color: var(--cyan); flex: none; }

/* ---------- Messages ---------- */
.messages { flex: 1; overflow-y: auto; padding: 24px 20px 8px; }
.loading-msg {
  text-align: center; color: var(--star-dim);
  padding: 80px 0; font-size: 14px;
  display: flex; align-items: center; justify-content: center; gap: 10px;
  font-family: var(--font-mono); letter-spacing: 0.05em;
}
.signal-pulse {
  width: 8px; height: 8px; border-radius: 50%;
  background: var(--gold);
  animation: pulse 1.6s ease-out infinite;
}
@keyframes pulse {
  0% { box-shadow: 0 0 0 0 rgba(230, 181, 102, 0.6); }
  70% { box-shadow: 0 0 0 12px rgba(230, 181, 102, 0); }
  100% { box-shadow: 0 0 0 0 rgba(230, 181, 102, 0); }
}

.empty-msg { text-align: center; color: var(--star-dim); padding: 90px 24px; font-size: 15px; }
.empty-glyph { display: block; font-size: 34px; color: var(--gold); opacity: 0.5; margin-bottom: 16px; font-style: normal; }
.empty-msg p { line-height: 1.7; }

/* System messages */
.system-msg { text-align: center; padding: 10px 0; }
.system-text {
  display: inline-block;
  font-size: 12px; color: var(--star-dim);
  background: var(--ink-700);
  border: 1px solid var(--ink-500);
  padding: 4px 14px; border-radius: 14px;
  font-family: var(--font-mono); letter-spacing: 0.03em;
}

/* Chat bubbles */
.msg-row { display: flex; margin-bottom: 18px; align-items: flex-start; }
.msg-user { flex-direction: row-reverse; }
.msg-char { flex-direction: row; }
.msg-avatar {
  flex-shrink: 0; margin: 0 12px;
  background: var(--ink-600); color: var(--gold);
  font-family: var(--font-serif); font-weight: 600;
  box-shadow: 0 0 0 1px var(--ink-500), 0 0 14px rgba(230, 181, 102, 0.15);
}
.msg-avatar.user-av { color: var(--star); background: var(--ink-500); box-shadow: 0 0 0 1px var(--ink-500); }
.msg-avatar.typing-av { color: var(--cyan); }

.msg-bubble {
  max-width: 70%;
  padding: 10px 16px 8px 18px;
  border-radius: 14px;
  position: relative;
  word-break: break-word;
}
.msg-bubble.user {
  background: linear-gradient(135deg, rgba(230, 181, 102, 0.16), rgba(230, 181, 102, 0.08));
  border: 1px solid rgba(230, 181, 102, 0.35);
  border-top-right-radius: 4px;
  color: var(--star);
}
.msg-bubble.char {
  background: var(--ink-700);
  border: 1px solid var(--ink-500);
  border-top-left-radius: 4px;
  color: var(--star);
}
.msg-bubble.char.tx-line::before { left: 8px; background: linear-gradient(to bottom, transparent, var(--cyan), transparent); opacity: 0.6; }
.msg-bubble.user.tx-line::before { left: auto; right: 8px; background: linear-gradient(to bottom, transparent, var(--gold), transparent); opacity: 0.7; }
.msg-sender {
  font-size: 12px; color: var(--gold);
  font-weight: 600; margin-bottom: 3px;
  letter-spacing: 0.04em;
}
.msg-text { font-size: 15px; line-height: 1.6; white-space: pre-wrap; }
.msg-text :deep(.mention) { color: var(--gold); font-weight: 600; }
.msg-time {
  font-size: 10px; color: var(--star-faint);
  margin-top: 5px; text-align: right;
  font-family: var(--font-mono); letter-spacing: 0.04em;
}

.msg-bubble.typing { padding: 14px 20px; }
.typing-dots { display: inline-flex; gap: 5px; align-items: center; }
.typing-dots span {
  width: 7px; height: 7px; border-radius: 50%;
  background: var(--gold); opacity: 0.3;
  animation: blink 1.4s infinite;
}
.typing-dots span:nth-child(2) { animation-delay: 0.2s; }
.typing-dots span:nth-child(3) { animation-delay: 0.4s; }
@keyframes blink { 0%, 60%, 100% { opacity: 0.25; transform: scale(0.85); } 30% { opacity: 1; transform: scale(1); } }

/* ---------- Input bar ---------- */
.input-bar {
  display: flex; padding: 14px 20px; gap: 12px;
  background: rgba(11, 16, 30, 0.72);
  backdrop-filter: blur(14px);
  -webkit-backdrop-filter: blur(14px);
  border-top: 1px solid var(--ink-500);
  flex-shrink: 0;
}
.send-btn { flex: none; }
.input-wrapper { flex: 1; position: relative; }

/* @mention dropdown */
.mention-list {
  position: absolute; bottom: 100%; left: 0; right: 0; margin-bottom: 6px;
  background: var(--ink-700);
  border: 1px solid var(--ink-500);
  border-radius: 12px;
  box-shadow: 0 -4px 24px rgba(0, 0, 0, 0.4);
  max-height: 220px; overflow-y: auto; z-index: 100;
  padding: 4px;
}
.mention-item {
  display: flex; align-items: center; gap: 10px;
  padding: 8px 12px; cursor: pointer;
  border-radius: 8px;
  transition: background 0.15s;
  color: var(--star-soft);
}
.mention-item:hover { background: var(--ink-500); color: var(--gold); }
.mention-empty { padding: 12px; text-align: center; color: var(--star-dim); font-size: 13px; }

@media (max-width: 640px) {
  .top-bar { padding: 10px 12px; gap: 10px; }
  .gname { font-size: 15px; }
  .mode-toggle span { padding: 5px 10px; font-size: 12px; }
  .msg-bubble { max-width: 82%; }
  .auto-banner { font-size: 12px; padding: 8px 14px; }
}
</style>
