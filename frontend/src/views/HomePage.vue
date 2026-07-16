<template>
  <div class="home-page">
    <!-- 右上角设置按钮 -->
    <div class="settings-btn" @click="goSettings" title="设置">
      <el-icon :size="28"><Setting /></el-icon>
    </div>

    <!-- 中央区域 -->
    <div class="home-center">
      <h1 class="app-title">ChronoChat</h1>
      <p class="app-subtitle">时空对话</p>

      <div class="entry-buttons">
        <!-- 角色列表入口 -->
        <div class="entry-card" @click="goCharacterList">
          <div class="entry-icon character-icon">
            <el-icon :size="64"><UserFilled /></el-icon>
          </div>
          <span class="entry-label">角色列表</span>
          <span class="entry-desc">与你的时空旅人对话</span>
        </div>

        <!-- 群聊列表入口 -->
        <div class="entry-card" @click="goGroupChatList">
          <div class="entry-icon group-icon">
            <el-icon :size="64"><ChatDotRound /></el-icon>
          </div>
          <span class="entry-label">群聊列表</span>
          <span class="entry-desc">时空茶话会</span>
        </div>
      </div>

      <!-- 后端连接状态 -->
      <div class="status-bar">
        <span v-if="backendOnline" class="status online">
          <el-icon><CircleCheck /></el-icon> 后端服务已连接
        </span>
        <span v-else-if="checking" class="status checking">
          <el-icon><Loading /></el-icon> 正在检测后端连接...
        </span>
        <span v-else class="status offline">
          <el-icon><WarningFilled /></el-icon> 后端服务未连接
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
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  position: relative;
}

.settings-btn {
  position: absolute;
  top: 24px;
  right: 32px;
  width: 48px;
  height: 48px;
  border-radius: 50%;
  background: rgba(255, 255, 255, 0.15);
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  color: white;
  transition: all 0.3s;
  backdrop-filter: blur(10px);
}
.settings-btn:hover {
  background: rgba(255, 255, 255, 0.3);
  transform: rotate(30deg);
}

.home-center {
  text-align: center;
}

.app-title {
  font-size: 48px;
  font-weight: 700;
  color: white;
  margin: 0 0 4px 0;
  letter-spacing: 4px;
  text-shadow: 0 2px 12px rgba(0,0,0,0.2);
}

.app-subtitle {
  font-size: 18px;
  color: rgba(255,255,255,0.75);
  margin: 0 0 48px 0;
  letter-spacing: 8px;
}

.entry-buttons {
  display: flex;
  gap: 40px;
  justify-content: center;
}

.entry-card {
  width: 200px;
  height: 200px;
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.12);
  backdrop-filter: blur(15px);
  border: 1px solid rgba(255,255,255,0.2);
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  transition: all 0.35s cubic-bezier(0.4, 0, 0.2, 1);
  gap: 12px;
}
.entry-card:hover {
  transform: translateY(-8px);
  background: rgba(255, 255, 255, 0.22);
  box-shadow: 0 16px 40px rgba(0,0,0,0.2);
}

.entry-icon {
  width: 80px;
  height: 80px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
}
.character-icon {
  background: linear-gradient(135deg, #43e97b, #38f9d7);
  color: white;
}
.group-icon {
  background: linear-gradient(135deg, #fa709a, #fee140);
  color: white;
}

.entry-label {
  font-size: 20px;
  font-weight: 600;
  color: white;
}

.entry-desc {
  font-size: 13px;
  color: rgba(255,255,255,0.65);
}

.status-bar {
  margin-top: 48px;
}

.status {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  font-size: 13px;
  padding: 6px 16px;
  border-radius: 20px;
}
.status.online {
  color: #67c23a;
  background: rgba(103,194,58,0.15);
}
.status.offline {
  color: #f56c6c;
  background: rgba(245,108,108,0.15);
}
.status.checking {
  color: #e6a23c;
  background: rgba(230,162,60,0.15);
}
</style>
