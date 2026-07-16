<template>
  <div class="manage-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle @click="goBack" />
      <span class="page-title">群聊管理</span>
    </div>
    <div class="placeholder">
      <el-icon :size="48" color="#c0c4cc"><Setting /></el-icon>
      <p>群聊管理功能将在后续阶段实现</p>
      <el-button @click="goChat">返回群聊</el-button>
    </div>
  </div>
</template>

<script setup>
import { useRouter } from 'vue-router'
import { ArrowLeft, Setting } from '@element-plus/icons-vue'

const router = useRouter()
function goBack() { router.push('/groupchats') }
function goChat() {
  const id = router.currentRoute.value.params.id
  router.push(`/groupchats/${id}/chat`)
}
</script>

<style scoped>
.manage-page{min-height:100vh;background:#f0f2f5}
.top-bar{display:flex;align-items:center;padding:16px 24px;background:white}
.page-title{margin-left:16px;font-size:20px;font-weight:600}
.placeholder{text-align:center;padding:120px 20px;color:#909399}
.placeholder p{margin:16px 0;font-size:15px}
</style>
