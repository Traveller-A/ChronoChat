<template>
  <div class="list-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goHome" />
      <span class="page-title">角色列表</span>
      <el-tooltip content="创建角色" placement="left">
        <el-button :icon="Plus" circle type="primary" class="add-btn" @click="goCreate" />
      </el-tooltip>
    </div>

    <!-- Empty state -->
    <div v-if="!loading && characters.length === 0" class="empty-state">
      <el-icon :size="64" color="#c0c4cc"><UserFilled /></el-icon>
      <p>还没有角色呢，点击右上角 + 号创建你的第一个时空旅人吧</p>
    </div>

    <!-- Character list -->
    <div v-else class="char-list">
      <div
        v-for="char in characters"
        :key="char.id"
        class="char-item"
        @click="goChat(char.id)"
      >
        <el-avatar :size="52" :src="getAvatarUrl(char.id)" class="char-avatar">
          {{ char.name.charAt(0) }}
        </el-avatar>
        <div class="char-info">
          <div class="char-name">{{ char.name }}</div>
          <div class="char-signature">{{ char.personality_signature || '还没有个性签名' }}</div>
        </div>
        <el-button :icon="Edit" circle size="small" class="edit-btn"
          @click.stop="goEdit(char.id)" />
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ArrowLeft, Plus, Edit, UserFilled } from '@element-plus/icons-vue'
import { listCharacters, getAvatarUrl } from '@/api'

const router = useRouter()
const characters = ref([])
const loading = ref(true)

onMounted(async () => {
  try {
    const res = await listCharacters()
    if (res.code === 0) characters.value = res.data || []
  } catch (err) {
    console.error('Failed to load characters:', err)
  } finally {
    loading.value = false
  }
})

function goHome() { router.push('/') }
function goCreate() { router.push('/characters/create') }
function goChat(id) { router.push(`/characters/${id}/chat`) }  // Phase 4
function goEdit(id) { router.push(`/characters/${id}/edit`) }
</script>

<style scoped>
.list-page { min-height: 100vh; background: #f0f2f5; }
.top-bar {
  display: flex; align-items: center; padding: 16px 24px;
  background: white; box-shadow: 0 1px 4px rgba(0,0,0,0.06);
}
.back-btn { border: none; background: #f5f5f5; }
.page-title { margin: 0 16px; flex: 1; font-size: 20px; font-weight: 600; }

.empty-state {
  text-align: center; padding: 120px 20px; color: #909399;
}
.empty-state p { margin-top: 20px; font-size: 15px; }

.char-list { max-width: 700px; margin: 16px auto; padding: 0 16px; }
.char-item {
  display: flex; align-items: center; padding: 16px; margin-bottom: 8px;
  background: white; border-radius: 12px; cursor: pointer;
  transition: all 0.2s; box-shadow: 0 1px 4px rgba(0,0,0,0.04);
}
.char-item:hover { transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.08); }
.char-avatar { margin-right: 14px; flex-shrink: 0; }
.char-info { flex: 1; min-width: 0; }
.char-name { font-size: 16px; font-weight: 600; color: #303133; }
.char-signature { font-size: 13px; color: #909399; margin-top: 4px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.edit-btn { flex-shrink: 0; }
</style>
