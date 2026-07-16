<template>
  <div class="group-chat" v-loading="loading">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle @click="goBack" />
      <span class="gname">{{ group.name || '群聊' }}</span>
      <el-tag :type="statusTagType" size="small">{{ statusText }}</el-tag>
    </div>
    <div class="messages" ref="mc">
      <div v-if="messages.length === 0 && !loading" class="empty-hint">
        还没有消息，在下方发送第一条消息吧
      </div>
      <div v-for="(m, i) in messages" :key="i" class="msg">
        <span class="sender">{{ m.sender }}：</span>
        <span class="content">{{ m.content }}</span>
      </div>
      <div v-if="waiting" class="msg typing">对方正在输入...</div>
    </div>
    <div class="input-bar">
      <el-select v-model="senderId" size="small" style="width:120px">
        <el-option label="我" value="" />
        <el-option v-for="m in members" :key="m.id" :label="m.name" :value="m.id" />
      </el-select>
      <el-input v-model="text" placeholder="输入消息..." @keyup.enter="send" :disabled="waiting" />
      <el-button type="primary" @click="send" :disabled="waiting || !text.trim()">发送</el-button>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, nextTick, computed } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ArrowLeft } from '@element-plus/icons-vue'
import { getGroup, getGroupHistory, sendGroupMessage } from '@/api'
import { ElMessage } from 'element-plus'

const router = useRouter()
const route = useRoute()
const gid = route.params.id

const group = ref({})
const members = ref([])
const messages = ref([])
const text = ref('')
const senderId = ref('')
const waiting = ref(false)
const loading = ref(true)
const mc = ref(null)

const statusText = computed(() =>
  group.value.status === 'open' ? '进行中' : group.value.status === 'paused' ? '已暂停' : '已关闭')
const statusTagType = computed(() =>
  group.value.status === 'open' ? 'success' : group.value.status === 'paused' ? 'warning' : 'info')

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
      // Parse "[Name]: content" lines
      hr.data.history.split('\n').filter(Boolean).forEach(line => {
        const m = line.match(/^\[(.+?)\]:\s*(.+)/)
        if (m) messages.value.push({ sender: m[1], content: m[2] })
      })
    }
  } catch (e) { ElMessage.error('加载失败') }
  finally { loading.value = false }
})

async function send() {
  const msg = text.value.trim()
  if (!msg || waiting.value) return
  text.value = ''
  const sName = senderId.value
    ? (members.value.find(m => m.id === senderId.value) || {}).name || 'Unknown'
    : '我'
  messages.value.push({ sender: sName, content: msg })
  waiting.value = true
  await scrollBottom()
  try {
    const res = await sendGroupMessage(gid, msg, senderId.value || '')
    if (res.code === 0 && res.data.reply) {
      messages.value.push({ sender: res.data.responder_name || 'AI', content: res.data.reply })
    }
  } catch (e) { ElMessage.error('发送失败') }
  finally { waiting.value = false; await scrollBottom() }
}

async function scrollBottom() {
  await nextTick()
  if (mc.value) mc.value.scrollTop = mc.value.scrollHeight
}
function goBack() { router.push('/groupchats') }
</script>

<style scoped>
.group-chat{height:100vh;display:flex;flex-direction:column;background:#ededed}
.top-bar{display:flex;align-items:center;padding:10px 16px;background:white;gap:10px}
.gname{flex:1;font-size:17px;font-weight:600}

.messages{flex:1;overflow-y:auto;padding:16px}
.empty-hint{text-align:center;color:#909399;padding:40px 0;font-size:14px}
.msg{padding:6px 0;line-height:1.6}
.sender{font-weight:600;color:#409eff}
.content{color:#303133}
.typing{color:#909399;font-style:italic}

.input-bar{display:flex;padding:10px 16px;background:white;gap:8px;border-top:1px solid #e0e0e0}
</style>
