<template>
  <div class="page list-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goHome" />
      <span class="page-title">角色列表</span>
      <el-tooltip content="创建角色" placement="left">
        <el-button :icon="Plus" circle type="primary" class="add-btn" @click="goCreate" />
      </el-tooltip>
    </div>

    <!-- Empty state -->
    <div v-if="!loading && characters.length === 0" class="empty-state">
      <div class="empty-icon"><el-icon :size="30"><UserFilled /></el-icon></div>
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
        <el-avatar :size="54" :src="getAvatarUrl(char.id)" class="char-avatar">
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
.list-page { padding-bottom: 80px; }

.top-bar {
  display: flex;
  align-items: center;
  gap: 14px;
  padding: 8px 0 32px;
}
.back-btn {
  background: var(--ink-700) !important;
  border-color: var(--ink-500) !important;
  color: var(--star-soft) !important;
}
.back-btn:hover { border-color: var(--gold-dim) !important; color: var(--gold) !important; }
.page-title {
  flex: 1;
  font-family: var(--font-serif);
  font-size: var(--fs-xl);
  font-weight: 600;
  color: var(--star);
  letter-spacing: 0.02em;
}
.add-btn { flex: none; }

.empty-state {
  text-align: center;
  padding: 90px 24px;
  color: var(--star-dim);
}
.empty-icon {
  width: 72px; height: 72px;
  margin: 0 auto 18px;
  display: grid; place-items: center;
  border-radius: 50%;
  border: 1px solid var(--ink-500);
  background: radial-gradient(circle at 50% 40%, var(--ink-600), var(--ink-800));
  color: var(--star-dim);
}
.empty-state p { font-size: 15px; line-height: 1.7; max-width: 340px; margin: 0 auto; }

.char-list { max-width: 720px; margin: 0 auto; display: flex; flex-direction: column; gap: 12px; }
.char-item {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 16px 18px;
  background: var(--ink-700);
  border: 1px solid var(--ink-500);
  border-radius: var(--radius);
  cursor: pointer;
  transition: all 0.25s ease;
}
.char-item:hover {
  transform: translateY(-2px);
  border-color: var(--gold-dim);
  box-shadow: var(--shadow-card);
}
.char-avatar {
  flex-shrink: 0;
  background: var(--ink-600);
  color: var(--gold);
  font-family: var(--font-serif);
  font-weight: 600;
  box-shadow: 0 0 0 2px var(--ink-700), 0 0 0 3px var(--gold-dim), 0 0 18px rgba(230, 181, 102, 0.2);
}
.char-info { flex: 1; min-width: 0; }
.char-name {
  font-family: var(--font-serif);
  font-size: 17px;
  font-weight: 600;
  color: var(--star);
  letter-spacing: 0.02em;
}
.char-signature {
  font-size: 13px;
  color: var(--star-dim);
  margin-top: 4px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.edit-btn {
  flex-shrink: 0;
  background: var(--ink-600) !important;
  border-color: var(--ink-500) !important;
  color: var(--star-dim) !important;
}
.edit-btn:hover { border-color: var(--gold-dim) !important; color: var(--gold) !important; }

@media (max-width: 600px) {
  .char-item { padding: 14px; gap: 12px; }
  .char-name { font-size: 16px; }
}
</style>
