<template>
  <div class="page create-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <span class="page-title">创建群聊</span>
    </div>

    <div class="form-card">
      <el-form label-position="top" size="large">
        <el-form-item label="群聊名称" required>
          <el-input v-model="name" placeholder="给群聊起个名字" />
        </el-form-item>
        <el-form-item label="选择成员（至少 2 位）">
          <div class="members">
            <div v-for="c in allChars" :key="c.id" class="member"
              :class="{ sel: selected.includes(c.id) }" @click="toggle(c.id)">
              <el-avatar :size="38" :src="getAvatarUrl(c.id)" class="m-avatar">{{ c.name.charAt(0) }}</el-avatar>
              <span class="m-name">{{ c.name }}</span>
              <el-icon v-if="selected.includes(c.id)" class="m-check"><Check /></el-icon>
            </div>
            <div v-if="allChars.length === 0" class="no-chars">还没有可选角色，请先创建角色</div>
          </div>
        </el-form-item>
        <div class="bottom">
          <el-button @click="goBack">返回</el-button>
          <el-button type="primary" @click="doCreate" :disabled="selected.length < 2 || !name">创建群聊</el-button>
        </div>
      </el-form>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ArrowLeft, Check } from '@element-plus/icons-vue'
import { listCharacters, getAvatarUrl, createGroup } from '@/api'
import { ElMessage, ElMessageBox } from 'element-plus'

const router = useRouter()
const name = ref('')
const allChars = ref([])
const selected = ref([])

onMounted(async () => {
  try { const r = await listCharacters(); if(r.code===0) allChars.value=r.data||[] } catch(e){}
})
function toggle(id) {
  const i = selected.value.indexOf(id)
  if(i>=0) selected.value.splice(i,1); else selected.value.push(id)
}
async function doCreate() {
  try {
    const res = await createGroup({ name: name.value, members_json: JSON.stringify(selected.value) })
    if(res.code===0){ ElMessage.success('群聊已创建'); router.push(`/groupchats/${res.data.id}/chat`) }
    else ElMessage.error(res.message)
  } catch(e){ ElMessage.error('创建失败') }
}
function goBack(){
  if(name.value || selected.value.length) {
    ElMessageBox.confirm('确定放弃创建吗？','提示',{type:'warning'}).then(()=>router.push('/groupchats')).catch(()=>{})
  } else router.push('/groupchats')
}
</script>

<style scoped>
.create-page { padding-bottom: 80px; }

.top-bar { display: flex; align-items: center; gap: 14px; padding: 8px 0 32px; }
.back-btn {
  background: var(--ink-700) !important; border-color: var(--ink-500) !important; color: var(--star-soft) !important;
}
.back-btn:hover { border-color: var(--gold-dim) !important; color: var(--gold) !important; }
.page-title {
  font-family: var(--font-serif); font-size: var(--fs-xl); font-weight: 600;
  color: var(--star); letter-spacing: 0.02em;
}

.form-card {
  max-width: 640px; margin: 0 auto; padding: 30px 32px;
  background: var(--ink-700); border: 1px solid var(--ink-500);
  border-radius: var(--radius); box-shadow: var(--shadow-card);
}

.members { display: flex; flex-wrap: wrap; gap: 10px; width: 100%; }
.member {
  display: flex; align-items: center; gap: 10px;
  padding: 8px 14px 8px 8px;
  border: 1px solid var(--ink-500); border-radius: 999px;
  background: var(--ink-800); cursor: pointer;
  transition: all 0.2s ease;
}
.member:hover { border-color: var(--gold-dim); }
.member.sel {
  border-color: var(--gold);
  background: rgba(230, 181, 102, 0.1);
  box-shadow: 0 0 0 1px var(--gold), 0 0 16px rgba(230, 181, 102, 0.2);
}
.m-avatar { background: var(--ink-600); color: var(--gold); font-family: var(--font-serif); font-weight: 600; flex-shrink: 0; }
.member.sel .m-avatar { box-shadow: 0 0 0 1px var(--gold); }
.m-name { font-size: 14px; color: var(--star-soft); }
.member.sel .m-name { color: var(--star); }
.m-check { color: var(--gold); font-size: 16px; }
.no-chars { width: 100%; text-align: center; color: var(--star-dim); font-size: 14px; padding: 24px; }

.bottom {
  display: flex; justify-content: center; gap: 16px;
  margin-top: 28px; padding-top: 22px; border-top: 1px solid var(--ink-500);
}
</style>
