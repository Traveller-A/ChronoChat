<template>
  <div class="page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <span class="page-title">创建群聊</span>
    </div>
    <div class="form">
      <el-form label-position="top" size="large">
        <el-form-item label="群聊名称" required>
          <el-input v-model="name" placeholder="给群聊起个名字" />
        </el-form-item>
        <el-form-item label="选择成员（至少2位）">
          <div class="members">
            <div v-for="c in allChars" :key="c.id" class="member"
              :class="{sel:selected.includes(c.id)}" @click="toggle(c.id)">
              <el-avatar :size="36" :src="getAvatarUrl(c.id)">{{ c.name.charAt(0) }}</el-avatar>
              <span>{{ c.name }}</span>
              <el-icon v-if="selected.includes(c.id)" color="#67c23a"><Check /></el-icon>
            </div>
          </div>
        </el-form-item>
        <div class="bottom">
          <el-button @click="goBack">返回</el-button>
          <el-button type="primary" @click="doCreate" :disabled="selected.length<2||!name">创建群聊</el-button>
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
import { ElMessage } from 'element-plus'

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
    ElMessage.confirm('确定放弃创建吗？','提示',{type:'warning'}).then(()=>router.push('/groupchats'))
  } else router.push('/groupchats')
}
</script>

<style scoped>
.page{min-height:100vh;background:#f0f2f5}
.top-bar{display:flex;align-items:center;padding:16px 24px;background:white}
.back-btn{border:none;background:#f5f5f5}
.page-title{margin-left:16px;font-size:20px;font-weight:600}
.form{max-width:600px;margin:24px auto;padding:24px;background:white;border-radius:12px}
.members{display:flex;flex-wrap:wrap;gap:8px}
.member{display:flex;align-items:center;gap:8px;padding:8px 12px;border:1px solid #e0e0e0;border-radius:8px;cursor:pointer}
.member.sel{border-color:#67c23a;background:#f0f9eb}
.bottom{display:flex;justify-content:center;gap:16px;margin-top:24px;padding-top:16px;border-top:1px solid #ebeef5}
</style>
