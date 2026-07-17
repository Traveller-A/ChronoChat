<template>
  <div class="page info-page" v-loading="loading">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <span class="page-title">{{ char.name }}</span>
      <el-button :icon="Edit" circle class="edit-btn" @click="goEdit" />
    </div>

    <div class="info-content" v-if="!loading">
      <!-- 只读信息区 -->
      <div class="info-card">
        <div class="profile-top">
          <el-avatar :size="84" :src="getAvatarUrl(charId)" shape="square" class="big-avatar">
            {{ char.name?.charAt(0) }}
          </el-avatar>
          <div class="profile-detail">
            <h2>{{ char.name }}</h2>
            <div class="tags">
              <el-tag v-if="char.gender" effect="dark">{{ char.gender === 'Male' ? '男' : char.gender === 'Female' ? '女' : char.gender }}</el-tag>
              <el-tag v-if="char.age" type="info" effect="dark">{{ char.age }}岁</el-tag>
              <el-tag v-if="char.birthday" type="warning" effect="dark">{{ char.birthday }}</el-tag>
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

      <!-- 核心记忆 -->
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

      <!-- 底部操作栏 -->
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
.info-page { padding-bottom: 80px; }

.top-bar { display: flex; align-items: center; gap: 14px; padding: 8px 0 32px; }
.back-btn, .edit-btn {
  background: var(--ink-700) !important; border-color: var(--ink-500) !important; color: var(--star-soft) !important;
}
.back-btn:hover, .edit-btn:hover { border-color: var(--gold-dim) !important; color: var(--gold) !important; }
.page-title {
  flex: 1; font-family: var(--font-serif); font-size: var(--fs-xl); font-weight: 600;
  color: var(--star); letter-spacing: 0.02em;
  white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
}

.info-content { max-width: 720px; margin: 0 auto; display: flex; flex-direction: column; gap: 16px; }
.info-card {
  background: var(--ink-700); border: 1px solid var(--ink-500);
  border-radius: var(--radius); padding: 26px 28px; box-shadow: var(--shadow-card);
}

.profile-top { display: flex; gap: 22px; align-items: center; }
.big-avatar {
  flex-shrink: 0; background: var(--ink-600); color: var(--gold);
  font-family: var(--font-serif); font-weight: 600; border-radius: 14px;
  box-shadow: 0 0 0 1px var(--ink-500), 0 0 24px rgba(230, 181, 102, 0.2);
}
.profile-detail h2 { margin: 0 0 10px; font-size: 24px; color: var(--star); }
.tags { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 10px; }
.signature { color: var(--star-dim); font-size: 14px; }

.section-header { display: flex; align-items: center; gap: 12px; margin-bottom: 18px; }
.section-header h3 { margin: 0; font-size: 16px; font-family: var(--font-serif); color: var(--star); }
.update-time { font-size: 12px; color: var(--star-faint); flex: 1; font-family: var(--font-mono); }

.personality-section { margin-top: 26px; padding-top: 22px; border-top: 1px solid var(--ink-500); }
.mbti-badge { display: flex; align-items: baseline; gap: 12px; margin-bottom: 14px; }
.mbti-label { font-size: 12px; color: var(--star-dim); font-family: var(--font-mono); letter-spacing: 0.1em; }
.mbti-value { font-size: 32px; font-weight: 700; color: var(--gold); letter-spacing: 3px; font-family: var(--font-mono); }
.trait-list { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 14px; }
.breakdown { font-size: 13px; color: var(--star-soft); line-height: 1.7; margin-bottom: 8px; }
.speaking { font-size: 13px; color: var(--star-soft); line-height: 1.7; }
.speaking strong { color: var(--gold); }
.no-analysis, .no-data { color: var(--star-dim); font-size: 14px; text-align: center; padding: 24px; }

.memory-timeline { position: relative; padding-left: 22px; }
.memory-timeline::before {
  content: ''; position: absolute; left: 5px; top: 4px; bottom: 4px; width: 1px;
  background: linear-gradient(to bottom, var(--gold-dim), var(--ink-500));
}
.memory-item { position: relative; margin-bottom: 16px; padding-left: 12px; }
.memory-dot {
  position: absolute; left: -21px; top: 6px; width: 9px; height: 9px; border-radius: 50%;
  background: var(--gold); box-shadow: 0 0 0 2px var(--ink-700), 0 0 10px rgba(230, 181, 102, 0.6);
}
.memory-text { font-size: 14px; color: var(--star-soft); line-height: 1.6; }

.user-desc { font-size: 14px; color: var(--star-soft); line-height: 1.9; white-space: pre-wrap; }

.bottom-actions { display: flex; justify-content: center; gap: 16px; padding: 12px 0 0; }

@media (max-width: 600px) {
  .profile-top { flex-direction: column; gap: 14px; text-align: center; align-items: center; }
  .tags, .signature { justify-content: center; }
}
</style>
