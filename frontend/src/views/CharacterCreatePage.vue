<template>
  <div class="page create-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="handleBack" />
      <span class="page-title">创建角色</span>
    </div>

    <div class="form-container">
      <el-form ref="formRef" :model="form" label-position="top" size="large">

        <!-- 基础信息 -->
        <div class="section-title">基础信息</div>

        <el-form-item label="头像">
          <div class="avatar-upload" @click="triggerUpload">
            <el-avatar :size="100" :src="avatarPreview" shape="square" class="avatar-box">
              <el-icon :size="36"><Plus /></el-icon>
            </el-avatar>
            <span class="avatar-hint">点击上传</span>
          </div>
          <input ref="fileInput" type="file" accept="image/*" hidden @change="onFileChange" />
        </el-form-item>

        <el-form-item label="姓名" required>
          <el-input v-model="form.name" placeholder="给角色起个名字" />
        </el-form-item>

        <div class="row-two">
          <el-form-item label="性别">
            <el-select v-model="form.gender" placeholder="选择性别" clearable style="width:100%">
              <el-option label="男" value="Male" /><el-option label="女" value="Female" /><el-option label="其他" value="Other" />
            </el-select>
          </el-form-item>
          <el-form-item label="年龄">
            <el-input-number v-model="form.age" :min="1" :max="999" style="width:100%" />
          </el-form-item>
        </div>

        <div class="row-two">
          <el-form-item label="生日">
            <div class="birthday-wrap">
              <el-date-picker v-model="form.birthday_date" type="date" placeholder="留空则为创建日期"
                format="YYYY-MM-DD" value-format="YYYY-MM-DD" style="flex:1"
                :disabled="birthdayNoYear" />
              <el-checkbox v-model="birthdayNoYear" size="small" class="bday-check">
                仅月日
              </el-checkbox>
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
          <el-form-item label="MBTI（选填）">
            <el-select v-model="form.mbti" placeholder="选择MBTI" clearable filterable style="width:100%">
              <el-option v-for="t in mbtiTypes" :key="t" :label="t" :value="t" />
            </el-select>
          </el-form-item>
        </div>

        <el-form-item label="个性签名（选填，显示在角色列表）">
          <el-input v-model="form.personality_signature" placeholder="一句话介绍这个角色" />
        </el-form-item>

        <!-- 角色眼中的你 -->
        <div class="section-title">角色眼中的你</div>
        <el-form-item label="角色对你的认知描述（选填）">
          <el-input v-model="form.user_description" type="textarea" :rows="3"
            placeholder="角色是如何看待你的？例如：'张三是我最信任的朋友，我们在大学相识...'" />
        </el-form-item>

        <!-- 投喂/背景 -->
        <div class="section-title">投喂 & 背景</div>
        <el-form-item label="角色故事描述">
          <el-input v-model="form.story_text" type="textarea" :rows="5"
            placeholder="描述角色的背景故事、性格特点...（创建后不可修改）" />
        </el-form-item>

        <el-form-item label="上传聊天截图、手写日记等（选填）">
          <div class="story-images">
            <div v-for="(img, idx) in storyImagePreviews" :key="idx" class="story-img-wrap">
              <img :src="img" class="story-img" />
              <el-icon class="remove-img" @click="removeStoryImage(idx)"><CircleCloseFilled /></el-icon>
            </div>
            <div class="story-upload-btn" @click="triggerStoryUpload" v-if="storyImagePreviews.length < 6">
              <el-icon :size="26"><Plus /></el-icon>
            </div>
          </div>
          <input ref="storyFileInput" type="file" accept="image/*" multiple hidden @change="onStoryFileChange" />
        </el-form-item>

        <!-- 角色专属 API -->
        <el-collapse class="api-collapse">
          <el-collapse-item title="角色专属 API 配置（选填，留空使用全局API）" name="api">
            <div class="config-grid">
              <div class="config-sub">
                <h4><el-icon><ChatLineSquare /></el-icon> 文本处理 API</h4>
                <el-form-item label="Base URL">
                  <el-input v-model="form.text_api_base_url" placeholder="留空使用全局" clearable />
                </el-form-item>
                <el-form-item label="API Key">
                  <el-input v-model="form.text_api_key" type="password" show-password
                    placeholder="留空使用全局Key" clearable />
                </el-form-item>
                <el-form-item label="Model">
                  <el-input v-model="form.text_model" placeholder="留空使用全局" clearable />
                </el-form-item>
              </div>
              <div class="config-sub">
                <h4><el-icon><PictureFilled /></el-icon> 多模态 API（选填）</h4>
                <el-form-item label="Base URL">
                  <el-input v-model="form.multimodal_api_base_url" placeholder="留空使用全局" clearable />
                </el-form-item>
                <el-form-item label="API Key">
                  <el-input v-model="form.multimodal_api_key" type="password" show-password
                    placeholder="留空使用全局Key" clearable />
                </el-form-item>
                <el-form-item label="Model">
                  <el-input v-model="form.multimodal_model" placeholder="留空使用全局" clearable />
                </el-form-item>
              </div>
            </div>
          </el-collapse-item>
        </el-collapse>

        <!-- 底部操作栏 -->
        <div class="bottom-actions">
          <el-button @click="handleBack">返回</el-button>
          <el-button @click="saveDraft" :loading="saving">保存但不创建</el-button>
          <el-button type="primary" @click="doCreate" :loading="creating">创建角色</el-button>
        </div>
      </el-form>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive } from 'vue'
import { useRouter } from 'vue-router'
import { ArrowLeft, Plus, CircleCloseFilled } from '@element-plus/icons-vue'
import { createCharacter as createCharApi, generateCharacterFiles } from '@/api'
import { ElMessage, ElMessageBox } from 'element-plus'

const router = useRouter()

const mbtiTypes = ['INTJ','INTP','ENTJ','ENTP','INFJ','INFP','ENFJ','ENFP',
                   'ISTJ','ISFJ','ESTJ','ESFJ','ISTP','ISFP','ESTP','ESFP']

const form = reactive({
  name: '', gender: '', age: null, birthday_date: '', mbti: '',
  personality_signature: '', user_description: '', story_text: '',
  text_api_base_url: '', text_api_key: '', text_model: '',
  multimodal_api_base_url: '', multimodal_api_key: '', multimodal_model: ''
})

const birthdayNoYear = ref(false)
const birthdayMonth = ref(null)
const birthdayDay = ref(null)

const avatarBase64 = ref('')
const avatarPreview = ref('')
const storyImagePreviews = ref([])
const storyImageBase64List = ref([])
const fileInput = ref(null)
const storyFileInput = ref(null)
const saving = ref(false)
const creating = ref(false)

function triggerUpload() { fileInput.value?.click() }
function onFileChange(e) {
  const file = e.target.files[0]
  if (!file) return
  const reader = new FileReader()
  reader.onload = () => { avatarBase64.value = reader.result; avatarPreview.value = reader.result }
  reader.readAsDataURL(file)
}

function triggerStoryUpload() { storyFileInput.value?.click() }
function onStoryFileChange(e) {
  for (const file of e.target.files) {
    const reader = new FileReader()
    reader.onload = () => {
      storyImagePreviews.value.push(reader.result)
      storyImageBase64List.value.push(reader.result)
    }
    reader.readAsDataURL(file)
  }
  storyFileInput.value.value = ''
}
function removeStoryImage(idx) {
  storyImagePreviews.value.splice(idx, 1); storyImageBase64List.value.splice(idx, 1)
}

function buildPayload() {
  let birthday = ''
  if (birthdayNoYear.value) {
    const m = birthdayMonth.value ? String(birthdayMonth.value).padStart(2, '0') : ''
    const d = birthdayDay.value ? String(birthdayDay.value).padStart(2, '0') : ''
    if (m && d) birthday = m + '-' + d
  } else if (form.birthday_date) {
    birthday = form.birthday_date
  }

  return {
    name: form.name, gender: form.gender, age: form.age || 0,
    birthday, mbti: form.mbti,
    personality_signature: form.personality_signature,
    user_description: form.user_description,
    story_text: form.story_text,
    story_images: JSON.stringify(storyImageBase64List.value),
    avatar_base64: avatarBase64.value || '',
    text_api_base_url: form.text_api_base_url,
    text_api_key: form.text_api_key,
    text_model: form.text_model,
    multimodal_api_base_url: form.multimodal_api_base_url,
    multimodal_api_key: form.multimodal_api_key,
    multimodal_model: form.multimodal_model
  }
}

async function saveDraft() {
  if (!form.name) { ElMessage.warning('请输入角色姓名'); return }
  saving.value = true
  try {
    const res = await createCharApi(buildPayload())
    if (res.code === 0) ElMessage.success('角色已保存（草稿）')
  } catch (err) { ElMessage.error('保存失败') }
  finally { saving.value = false }
}

async function doCreate() {
  if (!form.name) { ElMessage.warning('请输入角色姓名'); return }
  creating.value = true
  try {
    const res = await createCharApi(buildPayload())
    if (res.code === 0) {
      const charId = res.data.id
      ElMessage.success('角色创建成功，正在生成角色文件...')

      // Auto-generate character files via AI
      try {
        const genRes = await generateCharacterFiles(charId)
        if (genRes.code === 0) {
          const d = genRes.data
          const parts = []
          if (d.identity_generated) parts.push('身份')
          if (d.soul_generated) parts.push('性格')
          if (d.memory_generated) parts.push('记忆')
          if (d.user_generated) parts.push('关系')
          if (parts.length > 0) {
            ElMessage.success(`角色文件已生成（${parts.join('、')}）`)
          } else {
            ElMessage.warning('AI 生成完成，但未能解析出完整内容')
          }
        }
      } catch (genErr) {
        ElMessage.warning('角色已创建，但文件生成失败: ' + genErr.message)
      }

      router.push('/characters')
    }
  } catch (err) { ElMessage.error('创建失败') }
  finally { creating.value = false }
}

function handleBack() {
  if (form.name || avatarBase64.value || form.story_text) {
    ElMessageBox.confirm('确定放弃创建角色吗？', '提示', {
      confirmButtonText: '确定', cancelButtonText: '取消', type: 'warning'
    }).then(() => router.push('/characters'))
  } else { router.push('/characters') }
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

.form-container {
  max-width: 740px; margin: 0 auto; padding: 30px 32px;
  background: var(--ink-700); border: 1px solid var(--ink-500);
  border-radius: var(--radius); box-shadow: var(--shadow-card);
}

.section-title {
  font-family: var(--font-serif); font-size: 15px; font-weight: 600;
  color: var(--gold); margin: 10px 0 18px; padding-bottom: 10px;
  border-bottom: 1px solid var(--ink-500); letter-spacing: 0.04em;
}

.avatar-upload { text-align: center; cursor: pointer; }
.avatar-box {
  background: var(--ink-600); color: var(--star-dim);
  border-radius: 14px; box-shadow: 0 0 0 1px var(--ink-500);
  transition: all 0.2s ease;
}
.avatar-upload:hover .avatar-box { box-shadow: 0 0 0 1px var(--gold-dim), 0 0 18px rgba(230, 181, 102, 0.2); color: var(--gold); }
.avatar-hint { display: block; margin-top: 10px; font-size: 12px; color: var(--star-dim); font-family: var(--font-mono); letter-spacing: 0.04em; }

.row-two { display: grid; grid-template-columns: 1fr 1fr; gap: 18px; }
.birthday-wrap { display: flex; align-items: center; gap: 12px; width: 100%; }
.bday-check { flex-shrink: 0; }
.bday-monthday { display: flex; margin-top: 8px; }

.story-images { display: flex; flex-wrap: wrap; gap: 12px; }
.story-img-wrap { position: relative; width: 84px; height: 84px; border-radius: 10px; overflow: hidden; box-shadow: 0 0 0 1px var(--ink-500); }
.story-img { width: 100%; height: 100%; object-fit: cover; }
.remove-img {
  position: absolute; top: -2px; right: -2px;
  color: var(--rust); cursor: pointer;
  background: var(--ink-900); border-radius: 50%; font-size: 20px;
}
.story-upload-btn {
  width: 84px; height: 84px; border: 1px dashed var(--ink-500); border-radius: 10px;
  display: flex; align-items: center; justify-content: center; cursor: pointer; color: var(--star-dim);
  transition: all 0.2s ease;
}
.story-upload-btn:hover { border-color: var(--gold); color: var(--gold); }

.api-collapse { margin-top: 8px; }
.config-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 24px; }
.config-sub h4 { display: flex; align-items: center; gap: 7px; font-size: 14px; color: var(--star-soft); margin: 0 0 10px; }
.config-sub h4 .el-icon { color: var(--gold); }

.bottom-actions {
  display: flex; justify-content: center; gap: 14px; flex-wrap: wrap;
  padding-top: 26px; border-top: 1px solid var(--ink-500); margin-top: 26px;
}

@media (max-width: 640px) {
  .row-two, .config-grid { grid-template-columns: 1fr; }
  .form-container { padding: 22px 18px; }
}
</style>
