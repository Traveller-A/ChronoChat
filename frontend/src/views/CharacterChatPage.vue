<template>
  <div class="chat-page" :class="{ 'letter-mode': mode === 'letter' }">
    <!-- 顶部导航栏 -->
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <div class="who">
        <el-avatar :size="38" :src="getAvatarUrl(characterId)" class="top-avatar">
          {{ charName?.charAt(0) }}
        </el-avatar>
        <div class="who-meta">
          <span class="top-name">{{ charName }}</span>
          <span class="api-badge">
            <i class="dot" :class="apiSource"></i>{{ apiSource === 'character' ? '角色专属 API' : '全局 API' }}
          </span>
        </div>
      </div>

      <div class="top-right">
        <div class="mode-toggle">
          <span :class="{ active: mode === 'letter' }" @click="switchMode('letter')">
            <el-icon><Notebook /></el-icon> 书信
          </span>
          <span :class="{ active: mode === 'chat' }" @click="switchMode('chat')">
            <el-icon><ChatDotRound /></el-icon> 即时
          </span>
        </div>
        <el-button :icon="UserFilled" circle size="small" class="info-btn" @click="goInfo" title="角色信息" />
      </div>
    </div>

    <!-- 对话流 -->
    <div class="chat-messages" ref="msgContainer">
      <div v-if="loading" class="loading-msg">
        <span class="signal-pulse"></span> 正在接通信号…
      </div>
      <div v-if="!loading && messages.length === 0" class="empty-msg">
        <i class="empty-glyph">✦</i>
        <p>{{ mode === 'letter' ? '写第一封信，开启你们的书信往来吧' : '发送第一条消息，开始对话吧' }}</p>
      </div>

      <template v-for="(msg, idx) in messages" :key="idx">
        <!-- 即时模式气泡 -->
        <div v-if="mode === 'chat'"
          :class="['msg-row', msg.role === 'user' ? 'msg-user' : 'msg-char']">
          <el-avatar v-if="msg.role === 'char'" :size="38" :src="getAvatarUrl(characterId)" class="msg-avatar">
            {{ charName?.charAt(0) }}
          </el-avatar>
          <div class="msg-bubble tx-line" :class="msg.role">
            <div class="msg-text">{{ msg.content }}</div>
            <div class="msg-time">{{ msg.time }}</div>
          </div>
          <el-avatar v-if="msg.role === 'user'" :size="38" icon="UserFilled" class="msg-avatar user-av" />
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
        <el-avatar v-if="mode === 'chat'" :size="38" :src="getAvatarUrl(characterId)" class="msg-avatar">
          {{ charName?.charAt(0) }}
        </el-avatar>
        <div :class="mode === 'letter' ? 'letter-card char typing-letter' : 'msg-bubble char tx-line typing'">
          <span v-if="mode === 'chat'" class="typing-dots"><span></span><span></span><span></span></span>
          <span v-else class="typing-text">正在书写回信…</span>
        </div>
      </div>
    </div>

    <!-- 即时模式底部输入 -->
    <div v-if="mode === 'chat'" class="input-bar">
      <el-input v-model="inputText" placeholder="输入消息，回车发送…" @keyup.enter="doSendChat"
        :disabled="waiting" size="large" />
      <el-button type="primary" :icon="Promotion" size="large" @click="doSendChat"
        :disabled="waiting || !inputText.trim()" class="send-btn" />
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
            placeholder="在此书写信件内容…" resize="none" />
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
// Separate caches for each mode - never shared
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
.chat-page {
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: linear-gradient(180deg, rgba(26, 37, 64, 0.88), rgba(20, 29, 51, 0.94));
}
.chat-page.letter-mode {
  background: linear-gradient(180deg, rgba(42, 33, 24, 0.90), rgba(28, 22, 16, 0.95));
}

/* ---------- Top bar ---------- */
.top-bar {
  display: flex;
  align-items: center;
  gap: 14px;
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

.who { display: flex; align-items: center; gap: 12px; flex: 1; min-width: 0; }
.top-avatar {
  flex: none;
  box-shadow: 0 0 0 2px var(--ink-700), 0 0 0 3px var(--gold-dim), 0 0 18px rgba(230, 181, 102, 0.25);
  background: var(--ink-600);
  color: var(--gold);
  font-family: var(--font-serif);
  font-weight: 600;
}
.who-meta { display: flex; flex-direction: column; gap: 2px; min-width: 0; }
.top-name {
  font-family: var(--font-serif);
  font-size: 17px;
  font-weight: 600;
  color: var(--star);
  letter-spacing: 0.02em;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.api-badge {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  font-size: 11px;
  color: var(--star-dim);
  font-family: var(--font-mono);
  letter-spacing: 0.04em;
}
.api-badge .dot {
  width: 6px; height: 6px; border-radius: 50%;
  background: var(--cyan);
  box-shadow: 0 0 8px var(--cyan);
}
.api-badge .dot.character { background: var(--gold); box-shadow: 0 0 8px var(--gold); }

.top-right { display: flex; align-items: center; gap: 10px; flex: none; }
.info-btn {
  background: var(--ink-700) !important;
  border-color: var(--ink-500) !important;
  color: var(--star-soft) !important;
}
.info-btn:hover { border-color: var(--gold-dim) !important; color: var(--gold) !important; }

/* Mode toggle */
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

/* ---------- Messages ---------- */
.chat-messages { flex: 1; overflow-y: auto; padding: 24px 20px 8px; }
.letter-mode .chat-messages { padding: 32px 24px 8px; }

.loading-msg {
  text-align: center;
  color: var(--star-dim);
  padding: 80px 0;
  font-size: 14px;
  display: flex; align-items: center; justify-content: center; gap: 10px;
  font-family: var(--font-mono);
  letter-spacing: 0.05em;
}
.signal-pulse {
  width: 8px; height: 8px; border-radius: 50%;
  background: var(--gold);
  box-shadow: 0 0 0 0 rgba(230, 181, 102, 0.6);
  animation: pulse 1.6s ease-out infinite;
}
@keyframes pulse {
  0% { box-shadow: 0 0 0 0 rgba(230, 181, 102, 0.6); }
  70% { box-shadow: 0 0 0 12px rgba(230, 181, 102, 0); }
  100% { box-shadow: 0 0 0 0 rgba(230, 181, 102, 0); }
}

.empty-msg {
  text-align: center;
  color: var(--star-dim);
  padding: 90px 24px;
  font-size: 15px;
}
.empty-glyph {
  display: block;
  font-size: 34px;
  color: var(--gold);
  opacity: 0.5;
  margin-bottom: 16px;
  font-style: normal;
}
.empty-msg p { line-height: 1.7; }

/* ---------- Chat bubbles (transmissions) ---------- */
.msg-row { display: flex; margin-bottom: 18px; align-items: flex-start; }
.msg-user { flex-direction: row-reverse; }
.msg-char { flex-direction: row; }
.msg-avatar {
  flex-shrink: 0;
  margin: 0 12px;
  background: var(--ink-600);
  color: var(--gold);
  font-family: var(--font-serif);
  font-weight: 600;
  box-shadow: 0 0 0 1px var(--ink-500);
}
.msg-avatar.user-av { color: var(--star); background: var(--ink-500); }

.msg-bubble {
  max-width: 70%;
  padding: 11px 16px 9px 18px;
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
.msg-text { font-size: 15px; line-height: 1.6; white-space: pre-wrap; }
.msg-time {
  font-size: 10px;
  color: var(--star-faint);
  margin-top: 5px;
  text-align: right;
  font-family: var(--font-mono);
  letter-spacing: 0.04em;
}

.msg-bubble.typing { padding: 14px 20px; }
.typing-dots { display: inline-flex; gap: 5px; align-items: center; }
.typing-dots span {
  width: 7px; height: 7px; border-radius: 50%;
  background: var(--gold);
  opacity: 0.3;
  animation: blink 1.4s infinite;
}
.typing-dots span:nth-child(2) { animation-delay: 0.2s; }
.typing-dots span:nth-child(3) { animation-delay: 0.4s; }
@keyframes blink { 0%, 60%, 100% { opacity: 0.25; transform: scale(0.85); } 30% { opacity: 1; transform: scale(1); } }

/* ---------- Letter cards (parchment in the void) ---------- */
.letter-card {
  max-width: 78%;
  margin: 0 auto 28px;
  padding: 30px 36px;
  background: #e3cfa3;
  border: 1px solid rgba(138, 107, 58, 0.45);
  border-radius: 3px;
  box-shadow: 0 12px 40px rgba(0, 0, 0, 0.45), 0 0 0 1px rgba(138, 107, 58, 0.15) inset;
  position: relative;
  color: #2c2418;
}
.letter-card::after {
  content: '';
  position: absolute; inset: 6px;
  border: 1px solid rgba(138, 107, 58, 0.28);
  border-radius: 2px;
  pointer-events: none;
}
.letter-card.user { background: #d8c191; }
.letter-card.char { background: #e8d4a8; }
.letter-header {
  display: flex; justify-content: space-between; align-items: baseline;
  margin-bottom: 18px; padding-bottom: 12px;
  border-bottom: 1px solid rgba(138, 107, 58, 0.3);
}
.letter-role { font-family: var(--font-serif); font-size: 16px; font-weight: 600; color: #5d4e37; letter-spacing: 0.02em; }
.letter-date { font-size: 12px; color: #8a7350; font-family: var(--font-mono); letter-spacing: 0.04em; }
.letter-body {
  font-family: var(--font-serif);
  font-size: 15.5px; line-height: 2;
  color: #2c2418; white-space: pre-wrap; min-height: 60px;
}
.letter-footer {
  text-align: right; margin-top: 22px; padding-top: 14px;
  border-top: 1px solid rgba(138, 107, 58, 0.3);
  font-family: var(--font-serif);
  font-size: 15px; color: #5d4e37; font-style: italic;
}
.letter-card.typing-letter { padding: 22px 36px; text-align: center; }
.typing-text { color: #8a7350; font-family: var(--font-serif); font-style: italic; letter-spacing: 0.04em; }

/* ---------- Input bars ---------- */
.input-bar {
  display: flex; padding: 14px 20px; gap: 12px;
  background: rgba(11, 16, 30, 0.72);
  backdrop-filter: blur(14px);
  -webkit-backdrop-filter: blur(14px);
  border-top: 1px solid var(--ink-500);
  flex-shrink: 0;
}
.send-btn { flex: none; }

.letter-input-bar {
  padding: 18px 24px; text-align: center;
  background: rgba(11, 16, 30, 0.72);
  backdrop-filter: blur(14px);
  -webkit-backdrop-filter: blur(14px);
  border-top: 1px solid var(--ink-500);
  flex-shrink: 0;
}
.write-letter-btn { padding: 16px 56px; font-size: 15px; border-radius: 12px; letter-spacing: 0.06em; }

/* ---------- Letter editor dialog ---------- */
.letter-editor { max-width: 760px; margin: 0 auto; padding: 8px 4px; }
.editor-field { margin-bottom: 22px; }
.editor-field label {
  display: block; margin-bottom: 8px;
  font-size: 13px; color: var(--star-dim);
  font-weight: 500; letter-spacing: 0.06em;
  font-family: var(--font-mono);
  text-transform: uppercase;
}
.editor-field.row { display: grid; grid-template-columns: 1fr 1fr; gap: 18px; }
.editor-preview { margin-top: 28px; }
.editor-preview label {
  display: block; margin-bottom: 10px;
  font-size: 13px; color: var(--star-dim);
  font-family: var(--font-mono); letter-spacing: 0.06em; text-transform: uppercase;
}
.preview-card {
  padding: 28px 32px;
  background: #e3cfa3;
  border: 1px solid rgba(138, 107, 58, 0.45);
  border-radius: 3px;
  font-family: var(--font-serif);
  font-size: 15px; line-height: 2; color: #2c2418;
  box-shadow: inset 0 0 0 5px rgba(138, 107, 58, 0.12);
}
.preview-header { margin-bottom: 14px; font-weight: 600; }
.preview-footer { text-align: right; margin-top: 18px; padding-top: 10px; border-top: 1px solid rgba(138, 107, 58, 0.3); font-style: italic; color: #5d4e37; }

@media (max-width: 640px) {
  .top-bar { padding: 10px 12px; gap: 10px; }
  .top-name { font-size: 15px; }
  .api-badge { display: none; }
  .mode-toggle span { padding: 5px 10px; font-size: 12px; }
  .msg-bubble { max-width: 82%; }
  .letter-card { max-width: 92%; padding: 22px 22px; }
  .editor-field.row { grid-template-columns: 1fr; }
}
</style>
