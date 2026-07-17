<template>
  <div class="page edit-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="handleBack" />
      <span class="page-title">编辑角色</span>
    </div>

    <div class="form-container" v-loading="loading">
      <el-form label-position="top" size="large">

        <!-- 头像 -->
        <el-form-item label="头像">
          <div class="avatar-upload" @click="triggerUpload">
            <el-avatar :size="100" :src="avatarPreview || avatarSrc" shape="square" class="avatar-box">
              <el-icon :size="36"><Plus /></el-icon>
            </el-avatar>
            <span class="avatar-hint">点击更换头像</span>
          </div>
          <input ref="fileInput" type="file" accept="image/*" hidden @change="onFileChange" />
        </el-form-item>

        <!-- 姓名 & 性别：只读 -->
        <div class="row-two">
          <el-form-item label="姓名">
            <el-input :model-value="form.name" disabled />
            <span class="readonly-hint">创建后不可修改</span>
          </el-form-item>
          <el-form-item label="性别">
            <el-input :model-value="form.gender" disabled />
            <span class="readonly-hint">创建后不可修改</span>
          </el-form-item>
        </div>

        <!-- 年龄 & 生日 -->
        <div class="row-two">
          <el-form-item label="年龄">
            <el-input-number v-model="form.age" :min="1" :max="999" style="width:100%" />
          </el-form-item>
          <el-form-item label="生日">
            <div class="birthday-wrap">
              <el-date-picker v-model="form.birthday_date" type="date" format="YYYY-MM-DD"
                value-format="YYYY-MM-DD" style="flex:1" :disabled="birthdayNoYear" />
              <el-checkbox v-model="birthdayNoYear" size="small" class="bday-check">仅月日</el-checkbox>
            </div>
            <div v-if="birthdayNoYear" class="bday-monthday">
              <el-select v-model="birthdayMonth" placeholder="月" style="width:96px" clearable>
                <el-option v-for="m in 12" :key="m" :label="m+'月'" :value="m" />
              </el-select>
              <el-select v-model="birthdayDay" placeholder="日" style="width:96px;margin-left:8px" clearable>
                <el-option v-for="d in 31" :key="d" :label="d+'日'" :value="d" />
              </el-select>
            </div>
          </el-form-item>
        </div>

        <el-form-item label="MBTI">
          <el-select v-model="form.mbti" clearable filterable style="width:100%">
            <el-option v-for="t in mbtiTypes" :key="t" :label="t" :value="t" />
          </el-select>
        </el-form-item>

        <el-form-item label="个性签名">
          <el-input v-model="form.personality_signature" placeholder="简短的个性签名" />
        </el-form-item>

        <!-- 角色眼中的你 -->
        <el-form-item label="角色对你的认知描述">
          <el-input v-model="form.user_description" type="textarea" :rows="3"
            placeholder="角色是如何看待你的？" />
        </el-form-item>

        <!-- 角色专属 API -->
        <el-collapse class="api-collapse">
          <el-collapse-item title="角色专属 API 配置（留空使用全局API）" name="api">
            <div class="config-grid">
              <div class="config-sub">
                <h4><el-icon><ChatLineSquare /></el-icon> 文本处理 API</h4>
                <el-form-item label="Base URL">
                  <el-input v-model="form.text_api_base_url" placeholder="留空使用全局" clearable />
                </el-form-item>
                <el-form-item label="API Key">
                  <el-input v-model="form.text_api_key" type="password" show-password
                    :placeholder="textKeyPlaceholder" clearable />
                </el-form-item>
                <el-form-item label="Model">
                  <el-input v-model="form.text_model" placeholder="留空使用全局" clearable />
                </el-form-item>
                <el-button size="small" @click="testCharText" :loading="testingText">
                  测试文本API
                </el-button>
                <span v-if="testTextResult" :class="['test-msg', testTextResult.ok ? 'ok' : 'fail']">
                  {{ testTextResult.msg }}
                </span>
              </div>
              <div class="config-sub">
                <h4><el-icon><PictureFilled /></el-icon> 多模态 API（选填）</h4>
                <el-form-item label="Base URL">
                  <el-input v-model="form.multimodal_api_base_url" placeholder="留空使用全局" clearable />
                </el-form-item>
                <el-form-item label="API Key">
                  <el-input v-model="form.multimodal_api_key" type="password" show-password
                    :placeholder="multiKeyPlaceholder" clearable />
                </el-form-item>
                <el-form-item label="Model">
                  <el-input v-model="form.multimodal_model" placeholder="留空使用全局" clearable />
                </el-form-item>
                <el-button size="small" @click="testCharMulti" :loading="testingMulti"
                  :disabled="!form.multimodal_api_base_url || (!form.multimodal_api_key && !multiApiHasKey)">
                  测试多模态API
                </el-button>
                <span v-if="testMultiResult" :class="['test-msg', testMultiResult.ok ? 'ok' : 'fail']">
                  {{ testMultiResult.msg }}
                </span>
              </div>
            </div>
          </el-collapse-item>
        </el-collapse>

        <!-- 底部操作栏 -->
        <div class="bottom-actions">
          <el-button @click="handleBack">返回</el-button>
          <el-button type="warning" @click="regenerate" :loading="regenerating">重新生成角色文件</el-button>
          <el-button type="danger" @click="confirmDelete">删除角色</el-button>
          <el-button type="primary" @click="saveEdit" :loading="saving">保存修改</el-button>
        </div>
      </el-form>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ArrowLeft, Plus } from '@element-plus/icons-vue'
import { getCharacter, updateCharacter, deleteCharacter, generateCharacterFiles, getAvatarUrl } from '@/api'
import { ElMessage, ElMessageBox } from 'element-plus'
import apiClient from '@/api'

const router = useRouter()
const route = useRoute()
const characterId = route.params.id

const mbtiTypes = ['INTJ','INTP','ENTJ','ENTP','INFJ','INFP','ENFJ','ENFP',
                   'ISTJ','ISFJ','ESTJ','ESFJ','ISTP','ISFP','ESTP','ESFP']

const form = reactive({
  name: '', gender: '', age: null, birthday_date: '', mbti: '',
  personality_signature: '', user_description: '',
  text_api_base_url: '', text_api_key: '', text_model: '',
  multimodal_api_base_url: '', multimodal_api_key: '', multimodal_model: ''
})

const birthdayNoYear = ref(false)
const birthdayMonth = ref(null)
const birthdayDay = ref(null)

const avatarSrc = computed(() => getAvatarUrl(characterId))
const avatarBase64 = ref('')
const avatarPreview = ref('')
const loading = ref(true)
const saving = ref(false)
const regenerating = ref(false)

const textApiHasKey = ref(false)
const multiApiHasKey = ref(false)
const textKeyPlaceholder = ref('')
const multiKeyPlaceholder = ref('')
const testingText = ref(false)
const testingMulti = ref(false)
const testTextResult = ref(null)
const testMultiResult = ref(null)

onMounted(async () => {
  try {
    const res = await getCharacter(characterId)
    if (res.code === 0 && res.data) {
      const d = res.data
      form.name = d.name
      form.gender = d.gender === 'Male' ? '男' : d.gender === 'Female' ? '女' : d.gender || ''
      form.age = d.age
      // Parse birthday
      if (d.birthday && /^\d{2}-\d{2}$/.test(d.birthday)) {
        // MM-DD format
        birthdayNoYear.value = true
        const [m, d2] = d.birthday.split('-')
        birthdayMonth.value = parseInt(m)
        birthdayDay.value = parseInt(d2)
      } else if (d.birthday) {
        form.birthday_date = d.birthday
      }
      form.mbti = d.mbti || ''
      form.personality_signature = d.personality_signature || ''
      form.user_description = d.user_description || ''
      form.text_api_base_url = d.text_api_base_url || ''
      form.text_model = d.text_model || ''
      form.multimodal_api_base_url = d.multimodal_api_base_url || ''
      form.multimodal_model = d.multimodal_model || ''

      textApiHasKey.value = d.text_api_has_key
      multiApiHasKey.value = d.multimodal_api_has_key
      textKeyPlaceholder.value = d.text_api_has_key
        ? 'Key 已配置: ' + (d.text_api_key || '********') : '留空使用全局Key'
      multiKeyPlaceholder.value = d.multimodal_api_has_key
        ? 'Key 已配置: ' + (d.multimodal_api_key || '********') : '留空使用全局Key'

      // Never fill actual key into input
      form.text_api_key = ''
      form.multimodal_api_key = ''
    }
  } catch (err) { ElMessage.error('加载角色信息失败') }
  finally { loading.value = false }
})

function triggerUpload() { fileInput.value?.click() }
function onFileChange(e) {
  const file = e.target.files[0]
  if (!file) return
  const reader = new FileReader()
  reader.onload = () => { avatarBase64.value = reader.result; avatarPreview.value = reader.result }
  reader.readAsDataURL(file)
}

async function saveEdit() {
  saving.value = true
  try {
    let birthday = ''
    if (birthdayNoYear.value) {
      const m = birthdayMonth.value ? String(birthdayMonth.value).padStart(2, '0') : ''
      const d = birthdayDay.value ? String(birthdayDay.value).padStart(2, '0') : ''
      if (m && d) birthday = m + '-' + d
    } else if (form.birthday_date) {
      birthday = form.birthday_date
    }

    const payload = {
      age: form.age || 0,
      birthday,
      mbti: form.mbti,
      personality_signature: form.personality_signature,
      user_description: form.user_description,
      text_api_base_url: form.text_api_base_url,
      text_api_key: form.text_api_key,
      text_model: form.text_model,
      multimodal_api_base_url: form.multimodal_api_base_url,
      multimodal_api_key: form.multimodal_api_key,
      multimodal_model: form.multimodal_model
    }
    if (avatarBase64.value) payload.avatar_base64 = avatarBase64.value
    const res = await updateCharacter(characterId, payload)
    if (res.code === 0) {
      ElMessage.success('修改已保存')
      router.push('/characters')
    } else { ElMessage.error(res.message || '保存失败') }
  } catch (err) { ElMessage.error('保存失败') }
  finally { saving.value = false }
}

async function confirmDelete() {
  try {
    await ElMessageBox.confirm(
      '删除后该角色所有对话记录和记忆将永久丢失，是否继续？',
      '删除角色', { confirmButtonText: '删除', cancelButtonText: '取消', type: 'error' }
    )
    const res = await deleteCharacter(characterId)
    if (res.code === 0) {
      ElMessage.success('角色已删除')
      router.push('/characters')
    } else { ElMessage.error(res.message || '删除失败') }
  } catch (err) { /* cancelled */ }
}

async function testCharText() {
  testingText.value = true; testTextResult.value = null
  try {
    // Save current API config first
    await updateCharacter(characterId, {
      text_api_base_url: form.text_api_base_url,
      text_api_key: form.text_api_key,
      text_model: form.text_model
    })
    const res = await apiClient.post(`/characters/${encodeURIComponent(characterId)}/test-text`)
    testTextResult.value = { ok: res.code === 0, msg: res.message }
  } catch (err) { testTextResult.value = { ok: false, msg: '连接失败' } }
  finally { testingText.value = false }
}

async function testCharMulti() {
  testingMulti.value = true; testMultiResult.value = null
  try {
    await updateCharacter(characterId, {
      multimodal_api_base_url: form.multimodal_api_base_url,
      multimodal_api_key: form.multimodal_api_key,
      multimodal_model: form.multimodal_model
    })
    const res = await apiClient.post(`/characters/${encodeURIComponent(characterId)}/test-multimodal`)
    testMultiResult.value = { ok: res.code === 0, msg: res.message }
  } catch (err) { testMultiResult.value = { ok: false, msg: '连接失败' } }
  finally { testingMulti.value = false }
}

async function regenerate() {
  try {
    await ElMessageBox.confirm(
      '重新生成将覆盖当前的角色文件（IDENTITY/SOUL/MEMORY/USER），是否继续？',
      '重新生成', { confirmButtonText: '确定', cancelButtonText: '取消', type: 'warning' }
    )
  } catch { return }

  regenerating.value = true
  try {
    const res = await generateCharacterFiles(characterId)
    if (res.code === 0) {
      const d = res.data
      const parts = []
      if (d.identity_generated) parts.push('身份')
      if (d.soul_generated) parts.push('性格')
      if (d.memory_generated) parts.push('记忆')
      if (d.user_generated) parts.push('关系')
      ElMessage.success(parts.length > 0
        ? `文件已重新生成（${parts.join('、')}）`
        : '生成完成，但未能解析出完整内容')
    } else {
      ElMessage.error(res.message || '生成失败')
    }
  } catch (err) { ElMessage.error('生成失败') }
  finally { regenerating.value = false }
}

function handleBack() { router.push('/characters') }
</script>

<style scoped>
.edit-page { padding-bottom: 80px; }

.top-bar { display: flex; align-items: center; gap: 14px; padding: 8px 0 32px; }
.back-btn {
  background: var(--ink-700) !important; border-color: var(--ink-500) !important; color: var(--star-soft) !important;
}
.back-btn:hover { border-color: var(--gold-dim) !important; color: var(--gold) !important; }
.page-title {
  font-family: var(--font-serif); font-size: var(--fs-xl); font-weight: 600;
  color: var(--star); letter-spacing: 0.02em;
}

.form-container {
  max-width: 740px; margin: 0 auto; padding: 30px 32px;
  background: var(--ink-700); border: 1px solid var(--ink-500);
  border-radius: var(--radius); box-shadow: var(--shadow-card);
}

.avatar-upload { text-align: center; cursor: pointer; }
.avatar-box {
  background: var(--ink-600); color: var(--star-dim);
  border-radius: 14px; box-shadow: 0 0 0 1px var(--ink-500); transition: all 0.2s ease;
}
.avatar-upload:hover .avatar-box { box-shadow: 0 0 0 1px var(--gold-dim), 0 0 18px rgba(230, 181, 102, 0.2); color: var(--gold); }
.avatar-hint { display: block; margin-top: 10px; font-size: 12px; color: var(--star-dim); font-family: var(--font-mono); letter-spacing: 0.04em; }

.row-two { display: grid; grid-template-columns: 1fr 1fr; gap: 18px; }
.readonly-hint { font-size: 12px; color: var(--star-faint); font-family: var(--font-mono); }
.birthday-wrap { display: flex; align-items: center; gap: 12px; width: 100%; }
.bday-check { flex-shrink: 0; }
.bday-monthday { display: flex; margin-top: 8px; }

.api-collapse { margin-top: 8px; }
.config-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 24px; }
.config-sub h4 { display: flex; align-items: center; gap: 7px; font-size: 14px; color: var(--star-soft); margin: 0 0 10px; }
.config-sub h4 .el-icon { color: var(--gold); }
.test-msg { display: inline-block; margin-left: 10px; font-size: 12px; font-family: var(--font-mono); }
.test-msg.ok { color: var(--cyan); }
.test-msg.fail { color: var(--rust); }

.bottom-actions {
  display: flex; justify-content: center; gap: 12px; flex-wrap: wrap;
  padding-top: 26px; border-top: 1px solid var(--ink-500); margin-top: 26px;
}

@media (max-width: 640px) {
  .row-two, .config-grid { grid-template-columns: 1fr; }
  .form-container { padding: 22px 18px; }
}
</style>
