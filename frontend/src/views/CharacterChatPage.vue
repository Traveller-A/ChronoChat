<template>
  <div class="chat-page" :class="{ 'letter-mode': mode === 'letter' }">
    <!-- 顶部导航栏 -->
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <el-avatar :size="36" :src="getAvatarUrl(characterId)" class="top-avatar">
        {{ charName?.charAt(0) }}
      </el-avatar>
      <span class="top-name">{{ charName }}</span>

      <!-- 角色信息入口 -->
      <el-button :icon="UserFilled" circle size="small" class="info-btn" @click="goInfo" title="角色信息" />

      <!-- 模式切换开关 -->
      <div class="mode-toggle">
        <span :class="{ active: mode === 'letter' }" @click="switchMode('letter')">
          <el-icon><Notebook /></el-icon> 书信
        </span>
        <span :class="{ active: mode === 'chat' }" @click="switchMode('chat')">
          <el-icon><ChatDotRound /></el-icon> 即时
        </span>
      </div>

      <span class="api-badge">{{ apiSource === 'character' ? '角色专属API' : '全局API' }}</span>
    </div>

    <!-- 对话流 -->
    <div class="chat-messages" ref="msgContainer">
      <div v-if="loading" class="loading-msg">加载中...</div>
      <div v-if="!loading && messages.length === 0" class="empty-msg">
        {{ mode === 'letter' ? '写第一封信，开启你们的书信往来吧' : '发送第一条消息，开始对话吧' }}
      </div>

      <template v-for="(msg, idx) in messages" :key="idx">
        <!-- 即时模式气泡 -->
        <div v-if="mode === 'chat'"
          :class="['msg-row', msg.role === 'user' ? 'msg-user' : 'msg-char']">
          <el-avatar v-if="msg.role === 'char'" :size="36" :src="getAvatarUrl(characterId)" class="msg-avatar">
            {{ charName?.charAt(0) }}
          </el-avatar>
          <div class="msg-bubble" :class="msg.role">
            <div class="msg-text">{{ msg.content }}</div>
            <div class="msg-time">{{ msg.time }}</div>
          </div>
          <el-avatar v-if="msg.role === 'user'" :size="36" icon="UserFilled" class="msg-avatar" />
        </div>

        <!-- 书信模式卡片 -->
        <div v-else class="letter-card" :class="msg.role">
          <div class="letter-header">
            <span class="letter-role">{{ msg.role === 'user' ? '致 ' + charName : charName + ' 来信' }}</span>
            <span class="letter-date">{{ msg.time || msg.date }}</span>
          </div>
          <div class="letter-body">{{ msg.content }}</div>
          <div class="letter-footer">
            {{ msg.role === 'user' ? userName || '我' : charName }}
          </div>
        </div>
      </template>

      <!-- 等待回复 -->
      <div v-if="waiting" class="msg-row msg-char">
        <el-avatar v-if="mode === 'chat'" :size="36" :src="getAvatarUrl(characterId)" class="msg-avatar" />
        <div :class="mode === 'letter' ? 'letter-card char typing-letter' : 'msg-bubble char typing'">
          <span v-if="mode === 'chat'" class="typing-dots"><span>.</span><span>.</span><span>.</span></span>
          <span v-else class="typing-text">正在书写回信...</span>
        </div>
      </div>
    </div>

    <!-- 即时模式底部输入 -->
    <div v-if="mode === 'chat'" class="input-bar">
      <el-input v-model="inputText" placeholder="输入消息..." @keyup.enter="doSendChat"
        :disabled="waiting" size="large" />
      <el-button type="primary" :icon="Promotion" size="large" @click="doSendChat"
        :disabled="waiting || !inputText.trim()" />
    </div>

    <!-- 书信模式底部按钮 -->
    <div v-if="mode === 'letter'" class="letter-input-bar">
      <el-button type="primary" size="large" class="write-letter-btn" @click="openLetterEditor"
        :disabled="waiting">
        <el-icon><EditPen /></el-icon> 写回信
      </el-button>
    </div>

    <!-- 书信编辑器弹窗 -->
    <el-dialog v-model="letterEditorVisible" title="写回信" fullscreen :close-on-click-modal="false"
      class="letter-editor-dialog">
      <div class="letter-editor">
        <div class="editor-field">
          <label>称呼</label>
          <el-input v-model="letterSalutation" :placeholder="'亲爱的 ' + charName + '：'" size="large" />
        </div>
        <div class="editor-field">
          <label>正文</label>
          <el-input v-model="letterBody" type="textarea" :rows="12"
            placeholder="在此书写信件内容..." resize="none" />
        </div>
        <div class="editor-field row">
          <div>
            <label>落款</label>
            <el-input v-model="letterSignature" :placeholder="userName || '我'" />
          </div>
          <div>
            <label>日期</label>
            <el-input v-model="letterDate" :placeholder="todayStr" />
          </div>
        </div>
        <div class="editor-preview" v-if="letterBody">
          <label>预览</label>
          <div class="preview-card">
            <div class="preview-header">{{ letterSalutation || '亲爱的 ' + charName + '：' }}</div>
            <div class="preview-body">{{ letterBody }}</div>
            <div class="preview-footer">{{ letterSignature || userName || '我' }}<br>{{ letterDate || todayStr }}</div>
          </div>
        </div>
      </div>
      <template #footer>
        <el-button @click="letterEditorVisible = false">取消</el-button>
        <el-button type="primary" @click="sendLetter" :disabled="!letterBody.trim() || waiting">
          发送信件
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, nextTick } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ArrowLeft, Promotion, Notebook, ChatDotRound, EditPen, UserFilled } from '@element-plus/icons-vue'
import { getCharacter, sendMessage, getChatHistory, getAvatarUrl } from '@/api'
import { ElMessage } from 'element-plus'

const router = useRouter()
const route = useRoute()
const characterId = route.params.id

const mode = ref('chat')  // 'chat' | 'letter'
const charName = ref('')
const userName = ref('')
const apiSource = ref('global')
// Separate caches for each mode — never shared
const chatMessages = ref([])
const letterMessages = ref([])
// Computed: auto-switches based on mode
const messages = computed(() => mode.value === 'chat' ? chatMessages.value : letterMessages.value)
const inputText = ref('')
const waiting = ref(false)
const loading = ref(true)
const msgContainer = ref(null)

// Letter editor
const letterEditorVisible = ref(false)
const letterSalutation = ref('')
const letterBody = ref('')
const letterSignature = ref('')
const letterDate = ref('')
const todayStr = new Date().toISOString().slice(0, 10)

onMounted(async () => {
  try {
    const [charRes] = await Promise.all([
      getCharacter(characterId),
      loadHistory('chat')
    ])
    if (charRes.code === 0 && charRes.data) {
      charName.value = charRes.data.name
      userName.value = charRes.data.user_description || ''
    }
  } catch (err) { ElMessage.error('加载失败') }
  finally { loading.value = false; await scrollBottom() }
})

async function loadHistory(m) {
  try {
    const res = await getChatHistory(characterId, m)
    if (res.code === 0 && res.data) {
      apiSource.value = res.data.api_source || 'global'
      if (res.data.history) {
        const parsed = parseMemory(res.data.history)
        if (m === 'chat') chatMessages.value = parsed
        else letterMessages.value = parsed
      }
    }
  } catch (err) { /* ignore */ }
}

async function switchMode(m) {
  mode.value = m
  // Load history for this mode if not yet loaded
  const cache = m === 'chat' ? chatMessages.value : letterMessages.value
  if (cache.length === 0) {
    await loadHistory(m)
  }
  await scrollBottom()
}

function parseMemory(memory) {
  const blocks = memory.split('---')
  const parsed = []
  for (const block of blocks) {
    // [\s\S] matches across newlines; non-greedy to stop at next marker
    const userMatch = block.match(/\*\*User\*\*:\s*([\s\S]+?)(?=\n\*\*Character\*\*|$)/)
    const charMatch = block.match(/\*\*Character\*\*:\s*([\s\S]+)/)
    if (userMatch && charMatch) {
      parsed.push({ role: 'user', content: userMatch[1].trim(), time: '', date: '' })
      parsed.push({ role: 'char', content: charMatch[1].trim(), time: '', date: '' })
    }
  }
  return parsed
}

// ---- Instant chat ----
async function doSendChat() {
  const text = inputText.value.trim()
  if (!text || waiting.value) return
  const userMsg = { role: 'user', content: text, time: nowTime(), date: todayStr }
  chatMessages.value.push(userMsg)
  inputText.value = ''
  await sendToAI(text)
}

// ---- Letter mode ----
function openLetterEditor() {
  letterSalutation.value = '亲爱的 ' + charName.value + '：'
  letterBody.value = ''
  letterSignature.value = userName.value || ''
  letterDate.value = todayStr
  letterEditorVisible.value = true
}

async function sendLetter() {
  const body = letterBody.value.trim()
  if (!body || waiting.value) return

  const salutation = letterSalutation.value || '亲爱的 ' + charName.value + '：'
  const signature = letterSignature.value || userName.value || '我'
  const date = letterDate.value || todayStr
  const fullLetter = salutation + '\n\n' + body + '\n\n' + signature + '\n' + date

  const userMsg = {
    role: 'user', content: body, salutation, signature, date, time: nowTime()
  }
  letterMessages.value.push(userMsg)
  letterEditorVisible.value = false
  await sendToAI(fullLetter)
}

// ---- Shared ----
async function sendToAI(text) {
  waiting.value = true
  await scrollBottom()
  const currentMode = mode.value
  try {
    const res = await sendMessage(characterId, text, currentMode)
    if (res.code === 0 && res.data) {
      const charMsg = { role: 'char', content: res.data.message, time: nowTime(), date: todayStr }
      if (currentMode === 'chat') chatMessages.value.push(charMsg)
      else letterMessages.value.push(charMsg)
      if (res.data.api_source) apiSource.value = res.data.api_source
    } else {
      ElMessage.error(res.message || '发送失败')
    }
  } catch (err) {
    ElMessage.error('发送失败: ' + err.message)
  } finally {
    waiting.value = false
    await scrollBottom()
  }
}

function nowTime() {
  const d = new Date()
  return d.getHours().toString().padStart(2,'0') + ':' + d.getMinutes().toString().padStart(2,'0')
}

async function scrollBottom() {
  await nextTick()
  if (msgContainer.value) msgContainer.value.scrollTop = msgContainer.value.scrollHeight
}

function goBack() { router.push('/characters') }
function goInfo() { router.push(`/characters/${characterId}/info`) }
</script>

<style scoped>
.chat-page { height: 100vh; display: flex; flex-direction: column; background: #ededed; }
.chat-page.letter-mode { background: #f5efe0; }

/* Top bar */
.top-bar { display: flex; align-items: center; padding: 10px 16px; background: white; box-shadow: 0 1px 4px rgba(0,0,0,0.06); flex-shrink: 0; }
.letter-mode .top-bar { background: #faf6ee; border-bottom: 1px solid #e0d5c0; }
.back-btn { border: none; background: #f5f5f5; }
.top-avatar { margin-left: 10px; }
.top-name { margin-left: 10px; font-size: 17px; font-weight: 600; flex: 1; }

/* Mode toggle */
.mode-toggle { display: flex; background: #f0f0f0; border-radius: 20px; padding: 2px; margin-right: 12px; }
.mode-toggle span {
  display: flex; align-items: center; gap: 4px; padding: 4px 14px;
  border-radius: 18px; cursor: pointer; font-size: 13px; color: #909399; transition: all 0.2s;
}
.mode-toggle span.active { background: white; color: #303133; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
.api-badge { font-size: 11px; color: #909399; background: #f0f0f0; padding: 2px 8px; border-radius: 10px; }
.info-btn { margin-right: 8px; border: none; background: #f5f5f5; }

/* Messages container */
.chat-messages { flex: 1; overflow-y: auto; padding: 16px; }
.letter-mode .chat-messages { padding: 24px 32px; }
.loading-msg, .empty-msg { text-align: center; color: #909399; padding: 60px 0; font-size: 15px; }

/* Chat bubbles */
.msg-row { display: flex; margin-bottom: 18px; align-items: flex-start; }
.msg-user { flex-direction: row-reverse; }
.msg-char { flex-direction: row; }
.msg-avatar { flex-shrink: 0; margin: 0 10px; }
.msg-bubble { max-width: 70%; padding: 10px 14px; border-radius: 8px; position: relative; }
.msg-bubble.user { background: #95ec69; border-top-right-radius: 2px; }
.msg-bubble.char { background: white; border-top-left-radius: 2px; }
.msg-text { font-size: 15px; line-height: 1.5; white-space: pre-wrap; word-break: break-word; }
.msg-time { font-size: 11px; color: #999; margin-top: 4px; text-align: right; }
.msg-bubble.typing { padding: 12px 20px; }
.typing-dots span { display: inline-block; width: 6px; height: 6px; border-radius: 50%; background: #ccc; margin: 0 2px; animation: blink 1.4s infinite; }
.typing-dots span:nth-child(2) { animation-delay: 0.2s; }
.typing-dots span:nth-child(3) { animation-delay: 0.4s; }
@keyframes blink { 0%,60%,100% { opacity: 0.2; } 30% { opacity: 1; } }

/* Letter cards */
.letter-card {
  max-width: 85%; margin: 0 auto 24px; padding: 28px 32px;
  background: #fffef8; border: 1px solid #e0d5c0; border-radius: 4px;
  box-shadow: 0 2px 8px rgba(139,109,56,0.08); position: relative;
}
.letter-card::before {
  content: ''; position: absolute; top: -1px; left: -1px; right: -1px; bottom: -1px;
  border: 1px solid #d4c8a8; border-radius: 4px; pointer-events: none; opacity: 0.3;
}
.letter-card.user { background: #fafaf5; }
.letter-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px; padding-bottom: 12px; border-bottom: 1px solid #e0d5c0; }
.letter-role { font-size: 16px; font-weight: 600; color: #5d4e37; }
.letter-date { font-size: 13px; color: #a09080; }
.letter-body { font-size: 15px; line-height: 2; color: #3d3226; white-space: pre-wrap; min-height: 60px; }
.letter-footer { text-align: right; margin-top: 20px; padding-top: 12px; border-top: 1px solid #e0d5c0; font-size: 15px; color: #5d4e37; }
.letter-card.typing-letter { padding: 20px 32px; text-align: center; }
.typing-text { color: #a09080; font-style: italic; }

/* Input bar - chat */
.input-bar { display: flex; padding: 10px 16px; background: white; gap: 10px; border-top: 1px solid #e0e0e0; flex-shrink: 0; }

/* Input bar - letter */
.letter-input-bar { padding: 16px 24px; text-align: center; background: #faf6ee; border-top: 1px solid #e0d5c0; flex-shrink: 0; }
.write-letter-btn { padding: 14px 48px; font-size: 16px; border-radius: 8px; }

/* Letter editor dialog */
.letter-editor { max-width: 700px; margin: 0 auto; }
.editor-field { margin-bottom: 20px; }
.editor-field label { display: block; margin-bottom: 6px; font-size: 14px; color: #606266; font-weight: 500; }
.editor-field.row { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
.editor-preview { margin-top: 20px; }
.editor-preview label { display: block; margin-bottom: 8px; font-size: 14px; color: #909399; }
.preview-card { padding: 24px; background: #fffef8; border: 1px solid #e0d5c0; border-radius: 4px; font-size: 15px; line-height: 2; color: #3d3226; }
.preview-header { margin-bottom: 12px; font-weight: 600; }
.preview-footer { text-align: right; margin-top: 16px; padding-top: 8px; border-top: 1px solid #e0d5c0; }
</style>
