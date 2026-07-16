<template>
  <div class="chat-page">
    <!-- 顶部导航栏 -->
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <el-avatar :size="36" :src="getAvatarUrl(characterId)" class="top-avatar">
        {{ charName?.charAt(0) }}
      </el-avatar>
      <span class="top-name">{{ charName }}</span>
      <span class="api-badge">{{ apiSource === 'character' ? '角色专属API' : '全局API' }}</span>
    </div>

    <!-- 对话流 -->
    <div class="chat-messages" ref="msgContainer">
      <div v-if="loading" class="loading-msg">加载中...</div>
      <div v-if="!loading && messages.length === 0" class="empty-msg">
        发送第一条消息，开始对话吧
      </div>
      <div v-for="(msg, idx) in messages" :key="idx"
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
      <div v-if="waiting" class="msg-row msg-char">
        <el-avatar :size="36" :src="getAvatarUrl(characterId)" class="msg-avatar" />
        <div class="msg-bubble char typing">
          <span class="typing-dots"><span>.</span><span>.</span><span>.</span></span>
        </div>
      </div>
    </div>

    <!-- 底部输入区 -->
    <div class="input-bar">
      <el-input v-model="inputText" placeholder="输入消息..." @keyup.enter="doSend"
        :disabled="waiting" size="large" />
      <el-button type="primary" :icon="Promotion" size="large" @click="doSend"
        :disabled="waiting || !inputText.trim()" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, nextTick, watch } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ArrowLeft, Promotion } from '@element-plus/icons-vue'
import { getCharacter, sendMessage, getChatHistory, getAvatarUrl } from '@/api'
import { ElMessage } from 'element-plus'

const router = useRouter()
const route = useRoute()
const characterId = route.params.id

const charName = ref('')
const apiSource = ref('global')
const messages = ref([])
const inputText = ref('')
const waiting = ref(false)
const loading = ref(true)
const msgContainer = ref(null)

onMounted(async () => {
  try {
    const [charRes, histRes] = await Promise.all([
      getCharacter(characterId),
      getChatHistory(characterId)
    ])
    if (charRes.code === 0 && charRes.data) {
      charName.value = charRes.data.name
    }
    if (histRes.code === 0 && histRes.data) {
      apiSource.value = histRes.data.api_source || 'global'
      // Parse memory into messages
      if (histRes.data.memory) {
        parseMemory(histRes.data.memory)
      }
    }
  } catch (err) {
    ElMessage.error('加载失败')
  } finally { loading.value = false }
})

function parseMemory(memory) {
  // Parse "**User**: ... **Character**: ..." blocks
  const blocks = memory.split('---')
  const parsed = []
  for (const block of blocks) {
    const userMatch = block.match(/\*\*User\*\*:\s*(.+)/)
    const charMatch = block.match(/\*\*Character\*\*:\s*(.+)/)
    if (userMatch && charMatch) {
      parsed.push({ role: 'user', content: userMatch[1].trim(), time: '' })
      parsed.push({ role: 'char', content: charMatch[1].trim(), time: '' })
    }
  }
  if (parsed.length > 0) messages.value = parsed
}

async function doSend() {
  const text = inputText.value.trim()
  if (!text || waiting.value) return

  messages.value.push({ role: 'user', content: text, time: nowTime() })
  inputText.value = ''
  waiting.value = true
  await scrollBottom()

  try {
    const res = await sendMessage(characterId, text)
    if (res.code === 0 && res.data) {
      messages.value.push({ role: 'char', content: res.data.message, time: nowTime() })
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
  return d.getHours().toString().padStart(2,'0') + ':' +
         d.getMinutes().toString().padStart(2,'0')
}

async function scrollBottom() {
  await nextTick()
  if (msgContainer.value) {
    msgContainer.value.scrollTop = msgContainer.value.scrollHeight
  }
}

function goBack() { router.push('/characters') }
</script>

<style scoped>
.chat-page { height: 100vh; display: flex; flex-direction: column; background: #ededed; }
.top-bar {
  display: flex; align-items: center; padding: 10px 16px;
  background: white; box-shadow: 0 1px 4px rgba(0,0,0,0.06); flex-shrink: 0;
}
.back-btn { border: none; background: #f5f5f5; }
.top-avatar { margin-left: 10px; }
.top-name { margin-left: 10px; font-size: 17px; font-weight: 600; flex: 1; }
.api-badge { font-size: 11px; color: #909399; background: #f0f0f0; padding: 2px 8px; border-radius: 10px; }

.chat-messages { flex: 1; overflow-y: auto; padding: 16px; }
.loading-msg, .empty-msg { text-align: center; color: #909399; padding: 60px 0; font-size: 15px; }

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
.typing-dots span {
  display: inline-block; width: 6px; height: 6px; border-radius: 50%;
  background: #ccc; margin: 0 2px; animation: blink 1.4s infinite;
}
.typing-dots span:nth-child(2) { animation-delay: 0.2s; }
.typing-dots span:nth-child(3) { animation-delay: 0.4s; }
@keyframes blink { 0%,60%,100% { opacity: 0.2; } 30% { opacity: 1; } }

.input-bar {
  display: flex; padding: 10px 16px; background: white; gap: 10px;
  border-top: 1px solid #e0e0e0; flex-shrink: 0;
}
</style>
