<template>
  <div class="group-chat" v-loading="loading">
    <!-- 顶部导航栏 -->
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <span class="gname">{{ group.name || '群聊' }}</span>

      <!-- 模式切换开关 -->
      <div class="mode-toggle">
        <span :class="{ active: chatMode === 'mention' }" @click="switchMode('mention')">
          <el-icon><ChatDotRound /></el-icon> @提及
        </span>
        <span :class="{ active: chatMode === 'auto' }" @click="switchMode('auto')">
          <el-icon><Connection /></el-icon> 自动
        </span>
      </div>

      <el-tag :type="statusTagType" size="small">{{ statusText }}</el-tag>
    </div>

    <!-- 自动模式提示横幅 -->
    <div v-if="chatMode === 'auto'" class="auto-banner">
      <el-icon><InfoFilled /></el-icon>
      自动对话模式将在后续阶段实现，当前仅为 @提及模式
    </div>

    <!-- 消息区域 -->
    <div class="messages" ref="mc">
      <div v-if="loading" class="loading-msg">加载中...</div>
      <div v-if="!loading && messages.length === 0" class="empty-msg">
        发送第一条消息，开始群聊吧
      </div>

      <template v-for="(m, i) in messages" :key="i">
        <div :class="['msg-row', m.role === 'user' ? 'msg-user' : 'msg-char']">
          <el-avatar v-if="m.role === 'char'" :size="36" :src="getMemberAvatar(m.senderId)" class="msg-avatar">
            {{ m.sender?.charAt(0) }}
          </el-avatar>
          <div class="msg-bubble" :class="m.role">
            <div v-if="m.role === 'char'" class="msg-sender">{{ m.sender }}</div>
            <div class="msg-text" v-html="renderContent(m.content)"></div>
            <div class="msg-time">{{ m.time }}</div>
          </div>
          <el-avatar v-if="m.role === 'user'" :size="36" icon="UserFilled" class="msg-avatar" />
        </div>
      </template>

      <!-- 等待回复 -->
      <div v-if="waiting" class="msg-row msg-char">
        <el-avatar :size="36" icon="ChatDotRound" class="msg-avatar" />
        <div class="msg-bubble char typing">
          <span class="typing-dots"><span>.</span><span>.</span><span>.</span></span>
        </div>
      </div>
    </div>

    <!-- 底部输入 -->
    <div class="input-bar">
      <div class="input-wrapper">
        <!-- @自动补全下拉 -->
        <div v-if="showMentionList" class="mention-list">
          <div
            v-for="m in filteredMembers"
            :key="m.id"
            class="mention-item"
            @click="selectMention(m)"
          >
            <el-avatar :size="28" :src="getMemberAvatar(m.id)">{{ m.name?.charAt(0) }}</el-avatar>
            <span>{{ m.name }}</span>
          </div>
          <div v-if="filteredMembers.length === 0" class="mention-empty">无匹配成员</div>
        </div>
        <el-input
          ref="inputRef"
          v-model="text"
          placeholder="输入消息，@成员名可提及角色..."
          @keyup.enter="send"
          @input="onInput"
          :disabled="waiting"
          size="large"
        />
      </div>
      <el-button type="primary" :icon="Promotion" size="large" @click="send" :disabled="waiting || !text.trim()" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, nextTick, computed, watch } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ArrowLeft, Promotion, ChatDotRound, Connection, InfoFilled } from '@element-plus/icons-vue'
import { getGroup, getGroupHistory, sendGroupMessage, getAvatarUrl } from '@/api'
import { ElMessage } from 'element-plus'

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

const chatMode = ref('mention')  // 'mention' or 'auto'
const showMentionList = ref(false)
const mentionFilter = ref('')
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
  if (mode === 'auto') {
    ElMessage.info('自动对话模式将在后续阶段实现')
  }
  chatMode.value = mode
}

onMounted(async () => {
  try {
    const [gr, hr] = await Promise.all([
      getGroup(gid),
      getGroupHistory(gid)
    ])
    if (gr.code === 0) {
      group.value = gr.data
      members.value = gr.data.member_details || []
      if (gr.data.mode) chatMode.value = gr.data.mode
    }
    if (hr.code === 0 && hr.data.history) {
      hr.data.history.split('\n').filter(Boolean).forEach(line => {
        const m = line.match(/^\[(.+?)\]:\s*(.+)/)
        if (m) {
          const senderName = m[1]
          const content = m[2]
          const isUser = (senderName === 'Me')
          const member = members.value.find(mb => mb.name === senderName)
          messages.value.push({
            role: isUser ? 'user' : 'char',
            sender: senderName,
            senderId: member ? member.id : '',
            content,
            time: ''
          })
        }
      })
    }
  } catch (e) { ElMessage.error('Failed to load') }
  finally { loading.value = false }
})

// @自动补全逻辑
function onInput(val) {
  // Check if the user just typed @
  const cursorPos = inputRef.value?.$el?.querySelector('input')?.selectionStart
  if (cursorPos === undefined) {
    showMentionList.value = false
    return
  }
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
  // Replace the partial @mention with the full name
  const cursorPos = inputRef.value?.$el?.querySelector('input')?.selectionStart || 0
  const beforeCursor = text.value.slice(0, cursorPos)
  const afterCursor = text.value.slice(cursorPos)
  const atIndex = beforeCursor.lastIndexOf('@')
  const newText = beforeCursor.slice(0, atIndex) + '@' + member.name + ' ' + afterCursor
  text.value = newText
  showMentionList.value = false
  // Focus back on input
  nextTick(() => {
    const inp = inputRef.value?.$el?.querySelector('input')
    if (inp) {
      const newPos = atIndex + member.name.length + 2
      inp.setSelectionRange(newPos, newPos)
      inp.focus()
    }
  })
}

// 高亮 @mention
function renderContent(content) {
  if (!content) return ''
  return content.replace(/@([\w\u4e00-\u9fff]+)/g, '<span class="mention">@$1</span>')
}

async function send() {
  const msg = text.value.trim()
  if (!msg || waiting.value) return
  showMentionList.value = false
  text.value = ''
  const now = nowTime()
  messages.value.push({ role: 'user', sender: 'Me', senderId: '', content: msg, time: now })
  waiting.value = true
  await scrollBottom()
  try {
    const res = await sendGroupMessage(gid, msg, '')
    if (res.code === 0 && res.data.reply) {
      messages.value.push({
        role: 'char',
        sender: res.data.responder_name || 'AI',
        senderId: res.data.responder_id || '',
        content: res.data.reply,
        time: nowTime()
      })
    }
  } catch (e) { ElMessage.error('Send failed') }
  finally { waiting.value = false; await scrollBottom() }
}

function nowTime() {
  const d = new Date()
  return d.getHours().toString().padStart(2, '0') + ':' + d.getMinutes().toString().padStart(2, '0')
}

async function scrollBottom() {
  await nextTick()
  if (mc.value) mc.value.scrollTop = mc.value.scrollHeight
}
function goBack() { router.push('/groupchats') }
</script>

<style scoped>
.group-chat { height: 100vh; display: flex; flex-direction: column; background: #ededed; }

/* Top bar */
.top-bar { display: flex; align-items: center; padding: 10px 16px; background: white; box-shadow: 0 1px 4px rgba(0,0,0,0.06); flex-shrink: 0; gap: 10px; }
.gname { flex: 1; font-size: 17px; font-weight: 600; }
.back-btn { border: none; background: #f5f5f5; }

/* Mode toggle */
.mode-toggle { display: flex; background: #f0f0f0; border-radius: 20px; padding: 2px; margin-right: 4px; }
.mode-toggle span {
  display: flex; align-items: center; gap: 4px; padding: 4px 14px;
  border-radius: 18px; cursor: pointer; font-size: 13px; color: #909399; transition: all 0.2s;
}
.mode-toggle span.active { background: white; color: #303133; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }

/* Auto banner */
.auto-banner {
  display: flex; align-items: center; gap: 8px; padding: 8px 16px;
  background: #e6f7ff; color: #1890ff; font-size: 13px; border-bottom: 1px solid #91d5ff;
}

/* Messages container */
.messages { flex: 1; overflow-y: auto; padding: 16px; }
.loading-msg, .empty-msg { text-align: center; color: #909399; padding: 60px 0; font-size: 15px; }

/* Chat bubbles */
.msg-row { display: flex; margin-bottom: 18px; align-items: flex-start; }
.msg-user { flex-direction: row-reverse; }
.msg-char { flex-direction: row; }
.msg-avatar { flex-shrink: 0; margin: 0 10px; }
.msg-bubble { max-width: 70%; padding: 10px 14px; border-radius: 8px; position: relative; }
.msg-bubble.user { background: #95ec69; border-top-right-radius: 2px; }
.msg-bubble.char { background: white; border-top-left-radius: 2px; }
.msg-sender { font-size: 12px; color: #409eff; font-weight: 600; margin-bottom: 2px; }
.msg-text { font-size: 15px; line-height: 1.5; white-space: pre-wrap; word-break: break-word; }
.msg-text :deep(.mention) { color: #409eff; font-weight: 600; }
.msg-time { font-size: 11px; color: #999; margin-top: 4px; text-align: right; }

/* Typing indicator */
.msg-bubble.typing { padding: 12px 20px; }
.typing-dots span { display: inline-block; width: 6px; height: 6px; border-radius: 50%; background: #ccc; margin: 0 2px; animation: blink 1.4s infinite; }
.typing-dots span:nth-child(2) { animation-delay: 0.2s; }
.typing-dots span:nth-child(3) { animation-delay: 0.4s; }
@keyframes blink { 0%,60%,100% { opacity: 0.2; } 30% { opacity: 1; } }

/* Input bar */
.input-bar { display: flex; padding: 10px 16px; background: white; gap: 10px; border-top: 1px solid #e0e0e0; flex-shrink: 0; }
.input-bar .el-button { flex-shrink: 0; }
.input-wrapper { flex: 1; position: relative; }

/* @mention dropdown */
.mention-list {
  position: absolute; bottom: 100%; left: 0; right: 0; margin-bottom: 4px;
  background: white; border: 1px solid #e4e7ed; border-radius: 8px;
  box-shadow: 0 -2px 12px rgba(0,0,0,0.1); max-height: 200px; overflow-y: auto; z-index: 100;
}
.mention-item {
  display: flex; align-items: center; gap: 10px; padding: 8px 12px;
  cursor: pointer; transition: background 0.15s;
}
.mention-item:hover { background: #f5f7fa; }
.mention-item:first-child { border-radius: 8px 8px 0 0; }
.mention-item:last-child { border-radius: 0 0 8px 8px; }
.mention-empty { padding: 12px; text-align: center; color: #909399; font-size: 13px; }
</style>
