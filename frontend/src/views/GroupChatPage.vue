<template>
  <div class="group-chat" v-loading="loading">
    <!-- 顶部导航栏 -->
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <span class="gname">{{ group.name || '群聊' }}</span>
      <el-tag :type="statusTagType" size="small">{{ statusText }}</el-tag>
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
            <div class="msg-text">{{ m.content }}</div>
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
      <el-input v-model="text" placeholder="输入消息..." @keyup.enter="send" :disabled="waiting" size="large" />
      <el-button type="primary" :icon="Promotion" size="large" @click="send" :disabled="waiting || !text.trim()" />
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, nextTick, computed } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ArrowLeft, Promotion, ChatDotRound } from '@element-plus/icons-vue'
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

const statusText = computed(() =>
  group.value.status === 'open' ? '进行中' : group.value.status === 'paused' ? '已暂停' : '已关闭')
const statusTagType = computed(() =>
  group.value.status === 'open' ? 'success' : group.value.status === 'paused' ? 'warning' : 'info')

// 根据 senderId 获取成员头像 URL
function getMemberAvatar(senderId) {
  if (!senderId) return ''
  return getAvatarUrl(senderId)
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
    }
    if (hr.code === 0 && hr.data.history) {
      hr.data.history.split('\n').filter(Boolean).forEach(line => {
        const m = line.match(/^\[(.+?)\]:\s*(.+)/)
        if (m) {
          const senderName = m[1]
          const content = m[2]
          const isUser = (senderName === '我' || senderName === 'Me')
          // 查找对应成员
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
  } catch (e) { ElMessage.error('加载失败') }
  finally { loading.value = false }
})

async function send() {
  const msg = text.value.trim()
  if (!msg || waiting.value) return
  text.value = ''
  const now = nowTime()
  messages.value.push({ role: 'user', sender: '我', senderId: '', content: msg, time: now })
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
  } catch (e) { ElMessage.error('发送失败') }
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

/* Messages container */
.messages { flex: 1; overflow-y: auto; padding: 16px; }
.loading-msg, .empty-msg { text-align: center; color: #909399; padding: 60px 0; font-size: 15px; }

/* Chat bubbles (与角色对话即时模式一致) */
.msg-row { display: flex; margin-bottom: 18px; align-items: flex-start; }
.msg-user { flex-direction: row-reverse; }
.msg-char { flex-direction: row; }
.msg-avatar { flex-shrink: 0; margin: 0 10px; }
.msg-bubble { max-width: 70%; padding: 10px 14px; border-radius: 8px; position: relative; }
.msg-bubble.user { background: #95ec69; border-top-right-radius: 2px; }
.msg-bubble.char { background: white; border-top-left-radius: 2px; }
.msg-sender { font-size: 12px; color: #409eff; font-weight: 600; margin-bottom: 2px; }
.msg-text { font-size: 15px; line-height: 1.5; white-space: pre-wrap; word-break: break-word; }
.msg-time { font-size: 11px; color: #999; margin-top: 4px; text-align: right; }

/* Typing indicator */
.msg-bubble.typing { padding: 12px 20px; }
.typing-dots span { display: inline-block; width: 6px; height: 6px; border-radius: 50%; background: #ccc; margin: 0 2px; animation: blink 1.4s infinite; }
.typing-dots span:nth-child(2) { animation-delay: 0.2s; }
.typing-dots span:nth-child(3) { animation-delay: 0.4s; }
@keyframes blink { 0%,60%,100% { opacity: 0.2; } 30% { opacity: 1; } }

/* Input bar */
.input-bar { display: flex; padding: 10px 16px; background: white; gap: 10px; border-top: 1px solid #e0e0e0; flex-shrink: 0; }
.input-bar .el-input { flex: 1; }
</style>
