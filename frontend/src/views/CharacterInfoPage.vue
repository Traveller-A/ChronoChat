<template>
  <div class="info-page" v-loading="loading">
    <!-- 顶部导航栏 -->
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <span class="page-title">{{ char.name }}</span>
      <el-button :icon="Edit" circle class="edit-btn" @click="goEdit" />
    </div>

    <div class="info-content" v-if="!loading">
      <!-- ====== 只读信息区 ====== -->
      <div class="info-card">
        <div class="profile-top">
          <el-avatar :size="80" :src="getAvatarUrl(charId)" shape="square" class="big-avatar">
            {{ char.name?.charAt(0) }}
          </el-avatar>
          <div class="profile-detail">
            <h2>{{ char.name }}</h2>
            <div class="tags">
              <el-tag v-if="char.gender">{{ char.gender === 'Male' ? '男' : char.gender === 'Female' ? '女' : char.gender }}</el-tag>
              <el-tag v-if="char.age" type="info">{{ char.age }}岁</el-tag>
              <el-tag v-if="char.birthday" type="warning">{{ char.birthday }}</el-tag>
            </div>
            <p class="signature">{{ char.personality_signature || '还没有个性签名' }}</p>
          </div>
        </div>

        <!-- 人格分析 -->
        <div class="personality-section">
          <div class="section-header">
            <h3>人格分析</h3>
            <span class="update-time" v-if="analysisTime">上次更新: {{ analysisTime }}</span>
            <el-button size="small" type="primary" :loading="analyzing" @click="doAnalyze">
              重新推测
            </el-button>
          </div>
          <div v-if="analysis" class="analysis-result">
            <div class="mbti-badge" v-if="analysis.mbti">
              <span class="mbti-label">MBTI</span>
              <span class="mbti-value">{{ analysis.mbti }}</span>
            </div>
            <div v-if="analysis.traits" class="trait-list">
              <el-tag v-for="t in analysis.traits" :key="t" size="small" effect="plain">{{ t }}</el-tag>
            </div>
            <div v-if="analysis.mbti_breakdown" class="breakdown">{{ analysis.mbti_breakdown }}</div>
            <div v-if="analysis.speaking_style" class="speaking">
              <strong>说话风格：</strong>{{ analysis.speaking_style }}
            </div>
          </div>
          <div v-else class="no-analysis">点击"重新推测"进行人格分析</div>
        </div>
      </div>

      <!-- ====== 可变信息区 ====== -->
      <div class="info-card">
        <div class="section-header"><h3>核心记忆</h3></div>
        <div v-if="memoryFragments.length" class="memory-timeline">
          <div v-for="(item, idx) in memoryFragments" :key="idx" class="memory-item">
            <div class="memory-dot"></div>
            <div class="memory-text">{{ item }}</div>
          </div>
        </div>
        <div v-else class="no-data">暂无核心记忆</div>
      </div>

      <div class="info-card" v-if="char.user_description">
        <div class="section-header"><h3>角色眼中的你</h3></div>
        <p class="user-desc">{{ char.user_description }}</p>
      </div>

      <!-- ====== 底部操作栏 ====== -->
      <div class="bottom-actions">
        <el-button @click="doExport" :loading="exporting">导出</el-button>
        <el-button @click="doDuplicate" :loading="duplicating">存为副本</el-button>
        <el-button type="danger" @click="confirmDelete">删除角色</el-button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, computed } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ArrowLeft, Edit } from '@element-plus/icons-vue'
import { getCharacter, deleteCharacter, getAvatarUrl } from '@/api'
import apiClient from '@/api'
import { ElMessage, ElMessageBox } from 'element-plus'

const router = useRouter()
const route = useRoute()
const charId = route.params.id

const char = reactive({})
const loading = ref(true)
const analyzing = ref(false)
const exporting = ref(false)
const duplicating = ref(false)
const analysis = ref(null)
const analysisTime = ref('')

const memoryFragments = computed(() => {
  if (!char.memory_md) return []
  return char.memory_md.split('\n')
    .filter(line => line.trim().startsWith('-') || line.trim().startsWith('*'))
    .map(line => line.replace(/^[-*]\s*/, ''))
    .filter(Boolean)
    .slice(0, 20)
})

onMounted(async () => {
  try {
    const res = await getCharacter(charId)
    if (res.code === 0 && res.data) Object.assign(char, res.data)
  } catch (err) { ElMessage.error('加载失败') }
  finally { loading.value = false }
})

async function doAnalyze() {
  analyzing.value = true
  try {
    const res = await apiClient.post(`/characters/${encodeURIComponent(charId)}/analyze`)
    if (res.code === 0 && res.data) {
      analysis.value = res.data
      analysisTime.value = new Date().toLocaleString()
      ElMessage.success('人格分析完成')
    } else {
      ElMessage.error(res.message || '分析失败')
    }
  } catch (err) { ElMessage.error('分析失败') }
  finally { analyzing.value = false }
}

async function doExport() {
  exporting.value = true
  try {
    const res = await apiClient.get(`/characters/${encodeURIComponent(charId)}/export`)
    if (res.code === 0) {
      const blob = new Blob([JSON.stringify(res.data, null, 2)], { type: 'application/json' })
      const url = URL.createObjectURL(blob)
      const a = document.createElement('a')
      a.href = url; a.download = `${char.name}_export.json`; a.click()
      URL.revokeObjectURL(url)
      ElMessage.success('导出成功')
    }
  } catch (err) { ElMessage.error('导出失败') }
  finally { exporting.value = false }
}

async function doDuplicate() {
  duplicating.value = true
  try {
    const res = await apiClient.post(`/characters/${encodeURIComponent(charId)}/duplicate`)
    if (res.code === 0) {
      ElMessage.success('副本已创建')
      router.push('/characters')
    } else {
      ElMessage.error(res.message || '创建副本失败')
    }
  } catch (err) { ElMessage.error('创建副本失败') }
  finally { duplicating.value = false }
}

async function confirmDelete() {
  try {
    await ElMessageBox.confirm(
      '删除后该角色所有对话记录和记忆将永久丢失，是否继续？',
      '删除角色', { confirmButtonText: '删除', cancelButtonText: '取消', type: 'error' })
    const res = await deleteCharacter(charId)
    if (res.code === 0) { ElMessage.success('已删除'); router.push('/characters') }
    else ElMessage.error(res.message || '删除失败')
  } catch { /* cancelled */ }
}

function goBack() { router.push(`/characters/${charId}/chat`) }
function goEdit() { router.push(`/characters/${charId}/edit`) }
</script>

<style scoped>
.info-page { min-height: 100vh; background: #f0f2f5; }
.top-bar { display: flex; align-items: center; padding: 12px 20px; background: white; box-shadow: 0 1px 4px rgba(0,0,0,0.06); }
.back-btn { border: none; background: #f5f5f5; }
.page-title { flex: 1; margin: 0 14px; font-size: 19px; font-weight: 600; }
.edit-btn { border: none; background: #f5f5f5; }

.info-content { max-width: 700px; margin: 20px auto; padding: 0 16px; }
.info-card { background: white; border-radius: 12px; padding: 24px; margin-bottom: 16px; box-shadow: 0 1px 6px rgba(0,0,0,0.04); }

.profile-top { display: flex; gap: 20px; align-items: center; }
.big-avatar { flex-shrink: 0; }
.profile-detail h2 { margin: 0 0 8px; font-size: 22px; }
.tags { display: flex; gap: 6px; flex-wrap: wrap; margin-bottom: 8px; }
.signature { color: #606266; font-size: 14px; }

.section-header { display: flex; align-items: center; gap: 12px; margin-bottom: 16px; }
.section-header h3 { margin: 0; font-size: 16px; }
.update-time { font-size: 12px; color: #909399; flex: 1; }

.personality-section { margin-top: 24px; padding-top: 20px; border-top: 1px solid #ebeef5; }
.mbti-badge { display: flex; align-items: center; gap: 10px; margin-bottom: 12px; }
.mbti-label { font-size: 13px; color: #909399; }
.mbti-value { font-size: 28px; font-weight: 700; color: #409eff; letter-spacing: 2px; }
.trait-list { display: flex; gap: 6px; flex-wrap: wrap; margin-bottom: 12px; }
.breakdown { font-size: 13px; color: #606266; line-height: 1.6; margin-bottom: 8px; }
.speaking { font-size: 13px; color: #606266; }
.no-analysis, .no-data { color: #909399; font-size: 14px; text-align: center; padding: 20px; }

.memory-timeline { position: relative; padding-left: 20px; }
.memory-timeline::before { content: ''; position: absolute; left: 6px; top: 0; bottom: 0; width: 2px; background: #e0e0e0; }
.memory-item { position: relative; margin-bottom: 14px; padding-left: 10px; }
.memory-dot { position: absolute; left: -18px; top: 6px; width: 8px; height: 8px; border-radius: 50%; background: #409eff; }
.memory-text { font-size: 14px; color: #303133; line-height: 1.5; }

.user-desc { font-size: 14px; color: #606266; line-height: 1.8; white-space: pre-wrap; }

.bottom-actions { display: flex; justify-content: center; gap: 16px; padding: 8px 0 40px; }
</style>
