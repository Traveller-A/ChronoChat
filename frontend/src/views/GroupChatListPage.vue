<template>
  <div class="page list-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goHome" />
      <span class="page-title">群聊列表</span>
      <el-tooltip content="创建群聊" placement="left">
        <el-button :icon="Plus" circle type="primary" class="add-btn" @click="goCreate" />
      </el-tooltip>
    </div>

    <div v-if="groups.length===0" class="empty-state">
      <div class="empty-icon"><el-icon :size="30"><ChatDotRound /></el-icon></div>
      <p>还没有群聊，点击右上角 + 号创建一个时空茶话会吧</p>
    </div>

    <div v-else class="list">
      <div v-for="g in groups" :key="g.id" class="item">
        <div class="item-main" @click="goChat(g.id)">
          <el-avatar :size="50" shape="square" :src="getGroupAvatarUrl(g)" class="g-avatar">{{ g.name.charAt(0) }}</el-avatar>
          <div class="info">
            <div class="name">{{ g.name }}</div>
            <el-tag :type="g.status==='open'?'success':g.status==='paused'?'warning':'info'" size="small" effect="dark">
              {{ g.status==='open'?'进行中':g.status==='paused'?'已暂停':'已关闭' }}
            </el-tag>
          </div>
        </div>
        <div class="item-actions">
          <el-button :icon="Setting" circle size="small" class="act-btn" @click.stop="goManage(g.id)" title="管理" />
          <el-button :icon="Delete" circle size="small" type="danger" class="act-btn" @click.stop="confirmDelete(g)" title="删除" />
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ArrowLeft, Plus, ChatDotRound, Setting, Delete } from '@element-plus/icons-vue'
import { listGroups, deleteGroup } from '@/api'
import { ElMessage, ElMessageBox } from 'element-plus'

const router = useRouter()
const groups = ref([])
onMounted(async () => {
  try { const r = await listGroups(); if(r.code===0) groups.value=r.data||[] } catch(e){}
})
function goHome(){ router.push('/') }
function goCreate(){ router.push('/groupchats/create') }
function goChat(id){ router.push(`/groupchats/${id}/chat`) }
function goManage(id){ router.push(`/groupchats/${id}/manage`) }
function getGroupAvatarUrl(g) {
  if (g.avatar_path) return `/api/groups/${encodeURIComponent(g.id)}/avatar`
  return ''
}
async function confirmDelete(g) {
  try {
    await ElMessageBox.confirm(`Delete "${g.name}" and all its chat history?`, 'Delete Group', { type: 'error' })
    const res = await deleteGroup(g.id)
    if (res.code === 0) {
      ElMessage.success('Deleted')
      groups.value = groups.value.filter(x => x.id !== g.id)
    } else { ElMessage.error(res.message) }
  } catch (e) { /* cancelled */ }
}
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

.list { max-width: 720px; margin: 0 auto; display: flex; flex-direction: column; gap: 12px; }
.item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 14px 16px;
  background: var(--ink-700);
  border: 1px solid var(--ink-500);
  border-radius: var(--radius);
  transition: all 0.25s ease;
}
.item:hover {
  transform: translateY(-2px);
  border-color: var(--gold-dim);
  box-shadow: var(--shadow-card);
}
.item-main { display: flex; align-items: center; gap: 14px; flex: 1; cursor: pointer; min-width: 0; }
.g-avatar {
  flex-shrink: 0;
  background: var(--ink-600);
  color: var(--cyan);
  font-family: var(--font-serif);
  font-weight: 600;
  border-radius: 12px;
  box-shadow: 0 0 0 1px var(--ink-500), 0 0 16px rgba(94, 179, 196, 0.15);
}
.info { flex: 1; min-width: 0; display: flex; flex-direction: column; gap: 6px; }
.name {
  font-family: var(--font-serif);
  font-size: 17px;
  font-weight: 600;
  color: var(--star);
  letter-spacing: 0.02em;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
.item-actions { display: flex; gap: 8px; flex-shrink: 0; }
.act-btn {
  background: var(--ink-600) !important;
  border-color: var(--ink-500) !important;
  color: var(--star-dim) !important;
}
.act-btn:hover { border-color: var(--gold-dim) !important; color: var(--gold) !important; }

@media (max-width: 600px) {
  .item { padding: 12px; gap: 10px; }
  .name { font-size: 16px; }
}
</style>
