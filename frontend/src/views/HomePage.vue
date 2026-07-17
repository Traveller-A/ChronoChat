<template>
  <div class="home-page">
    <button class="settings-btn" @click="goSettings" title="设置" aria-label="设置">
      <el-icon :size="20"><Setting /></el-icon>
    </button>

    <div class="home-center">
      <div class="brand">
        <i class="brand-glyph">✦</i>
        <h1 class="app-title">ChronoChat</h1>
        <p class="app-subtitle">时空对话</p>
      </div>

      <div class="entry-buttons">
        <div class="entry-card" @click="goCharacterList">
          <div class="entry-orb character">
            <span class="orb-ring"></span>
            <el-icon :size="38"><UserFilled /></el-icon>
          </div>
          <span class="entry-label">角色列表</span>
          <span class="entry-desc">与你的时空旅人对话</span>
        </div>

        <div class="entry-card" @click="goGroupChatList">
          <div class="entry-orb group">
            <span class="orb-ring"></span>
            <el-icon :size="38"><ChatDotRound /></el-icon>
          </div>
          <span class="entry-label">群聊列表</span>
          <span class="entry-desc">时空茶话会</span>
        </div>
      </div>

      <div class="status-bar">
        <span v-if="backendOnline" class="status online">
          <span class="signal-dot"></span> 后端服务已连接
        </span>
        <span v-else-if="checking" class="status checking">
          <span class="signal-dot"></span> 正在检测后端连接…
        </span>
        <span v-else class="status offline">
          <span class="signal-dot"></span> 后端服务未连接
        </span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { hello } from '@/api'

const router = useRouter()
const backendOnline = ref(false)
const checking = ref(true)

// 检测后端连通性
onMounted(async () => {
  try {
    const res = await hello()
    if (res.code === 0) {
      backendOnline.value = true
      console.log('[ChronoChat] 后端连接成功:', res.message)
    }
  } catch (err) {
    backendOnline.value = false
    console.warn('[ChronoChat] 后端连接失败:', err.message)
  } finally {
    checking.value = false
  }
})

function goSettings() {
  router.push('/settings')
}

function goCharacterList() {
  router.push('/characters')
}

function goGroupChatList() {
  router.push('/groupchats')
}
</script>

<style scoped>
.home-page {
  width: 100%;
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  background: transparent;
  padding: 40px 24px;
}

.settings-btn {
  position: absolute;
  top: 26px;
  right: 32px;
  width: 46px;
  height: 46px;
  border-radius: 50%;
  border: 1px solid var(--ink-500);
  background: rgba(19, 26, 46, 0.6);
  backdrop-filter: blur(10px);
  -webkit-backdrop-filter: blur(10px);
  display: grid;
  place-items: center;
  cursor: pointer;
  color: var(--star-dim);
  transition: all 0.35s ease;
}
.settings-btn:hover {
  color: var(--gold);
  border-color: var(--gold-dim);
  transform: rotate(45deg);
  box-shadow: 0 0 18px rgba(230, 181, 102, 0.25);
}

.home-center { text-align: center; max-width: 720px; }

.brand { margin-bottom: 64px; }
.brand-glyph {
  display: block;
  font-size: 30px;
  color: var(--gold);
  opacity: 0.7;
  margin-bottom: 14px;
  font-style: normal;
  text-shadow: 0 0 24px rgba(230, 181, 102, 0.5);
}
.app-title {
  font-family: var(--font-serif);
  font-size: 64px;
  font-weight: 700;
  margin: 0 0 6px;
  letter-spacing: 0.04em;
  background: linear-gradient(135deg, var(--star) 30%, var(--gold) 100%);
  -webkit-background-clip: text;
  background-clip: text;
  -webkit-text-fill-color: transparent;
}
.app-subtitle {
  font-size: 16px;
  color: var(--star-dim);
  margin: 0;
  letter-spacing: 0.6em;
  padding-left: 0.6em;
}

.entry-buttons {
  display: flex;
  gap: 56px;
  justify-content: center;
  margin-bottom: 56px;
}

.entry-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 14px;
  cursor: pointer;
  padding: 12px;
  transition: transform 0.35s cubic-bezier(0.4, 0, 0.2, 1);
}
.entry-card:hover { transform: translateY(-6px); }

.entry-orb {
  width: 110px;
  height: 110px;
  border-radius: 50%;
  display: grid;
  place-items: center;
  position: relative;
  color: #1a1408;
  transition: box-shadow 0.4s ease;
}
.entry-orb.character {
  background: radial-gradient(circle at 35% 30%, var(--gold-bright), var(--gold) 55%, var(--gold-dim) 100%);
  box-shadow: 0 0 0 1px rgba(230, 181, 102, 0.4), 0 0 40px rgba(230, 181, 102, 0.35);
}
.entry-orb.group {
  background: radial-gradient(circle at 35% 30%, #7fcad9, var(--cyan) 55%, var(--cyan-dim) 100%);
  box-shadow: 0 0 0 1px rgba(94, 179, 196, 0.4), 0 0 40px rgba(94, 179, 196, 0.3);
  color: #06222a;
}
.entry-card:hover .entry-orb.character { box-shadow: 0 0 0 1px var(--gold), 0 0 60px rgba(230, 181, 102, 0.55); }
.entry-card:hover .entry-orb.group { box-shadow: 0 0 0 1px var(--cyan), 0 0 60px rgba(94, 179, 196, 0.5); }

.orb-ring {
  position: absolute;
  inset: -10px;
  border-radius: 50%;
  border: 1px solid rgba(230, 181, 102, 0.25);
  opacity: 0;
  transition: opacity 0.4s ease;
}
.entry-orb.group .orb-ring { border-color: rgba(94, 179, 196, 0.25); }
.entry-card:hover .orb-ring { opacity: 1; }

.entry-label {
  font-family: var(--font-serif);
  font-size: 20px;
  font-weight: 600;
  color: var(--star);
  letter-spacing: 0.04em;
}
.entry-desc {
  font-size: 13px;
  color: var(--star-dim);
  letter-spacing: 0.02em;
}

.status-bar { display: flex; justify-content: center; }
.status {
  display: inline-flex;
  align-items: center;
  gap: 9px;
  font-size: 13px;
  padding: 7px 18px;
  border-radius: 20px;
  border: 1px solid var(--ink-500);
  background: rgba(19, 26, 46, 0.5);
  font-family: var(--font-mono);
  letter-spacing: 0.04em;
}
.signal-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
}
.status.online { color: var(--cyan); }
.status.online .signal-dot { background: var(--cyan); box-shadow: 0 0 10px var(--cyan); }
.status.checking { color: var(--gold); }
.status.checking .signal-dot { background: var(--gold); box-shadow: 0 0 10px var(--gold); animation: pulse 1.6s ease-out infinite; }
.status.offline { color: var(--rust); }
.status.offline .signal-dot { background: var(--rust); }

@keyframes pulse {
  0% { box-shadow: 0 0 0 0 rgba(230, 181, 102, 0.6); }
  70% { box-shadow: 0 0 0 8px rgba(230, 181, 102, 0); }
  100% { box-shadow: 0 0 0 0 rgba(230, 181, 102, 0); }
}

@media (max-width: 600px) {
  .app-title { font-size: 44px; }
  .entry-buttons { gap: 28px; }
  .entry-orb { width: 88px; height: 88px; }
  .brand { margin-bottom: 44px; }
}
</style>
