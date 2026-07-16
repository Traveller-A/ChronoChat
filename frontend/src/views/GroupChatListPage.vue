<template>
  <div class="list-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goHome" />
      <span class="page-title">群聊列表</span>
      <el-tooltip content="创建群聊" placement="left">
        <el-button :icon="Plus" circle type="primary" @click="goCreate" />
      </el-tooltip>
    </div>
    <div v-if="groups.length===0" class="empty">
      <el-icon :size="64" color="#c0c4cc"><ChatDotRound /></el-icon>
      <p>还没有群聊，点击右上角+号创建一个时空茶话会吧</p>
    </div>
    <div v-else class="list">
      <div v-for="g in groups" :key="g.id" class="item" @click="goChat(g.id)">
        <el-avatar :size="48" shape="square">{{ g.name.charAt(0) }}</el-avatar>
        <div class="info">
          <div class="name">{{ g.name }}</div>
          <el-tag :type="g.status==='open'?'success':g.status==='paused'?'warning':'info'" size="small">
            {{ g.status==='open'?'进行中':g.status==='paused'?'已暂停':'已关闭' }}
          </el-tag>
        </div>
        <span class="arrow">›</span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ArrowLeft, Plus, ChatDotRound } from '@element-plus/icons-vue'
import { listGroups } from '@/api'

const router = useRouter()
const groups = ref([])
onMounted(async () => {
  try { const r = await listGroups(); if(r.code===0) groups.value=r.data||[] } catch(e){}
})
function goHome(){ router.push('/') }
function goCreate(){ router.push('/groupchats/create') }
function goChat(id){ router.push(`/groupchats/${id}/chat`) }
</script>

<style scoped>
.list-page{min-height:100vh;background:#f0f2f5}
.top-bar{display:flex;align-items:center;padding:16px 24px;background:white;box-shadow:0 1px 4px rgba(0,0,0,0.06)}
.back-btn{border:none;background:#f5f5f5}
.page-title{flex:1;margin:0 16px;font-size:20px;font-weight:600}
.empty{text-align:center;padding:120px 20px;color:#909399}
.empty p{margin-top:20px;font-size:15px}
.list{max-width:700px;margin:16px auto;padding:0 16px}
.item{display:flex;align-items:center;padding:14px;margin-bottom:8px;background:white;border-radius:12px;cursor:pointer;box-shadow:0 1px 4px rgba(0,0,0,0.04);transition:all .2s}
.item:hover{transform:translateY(-2px);box-shadow:0 4px 12px rgba(0,0,0,0.08)}
.info{flex:1;margin-left:14px}
.name{font-size:16px;font-weight:600}
.arrow{font-size:24px;color:#c0c4cc}
</style>
