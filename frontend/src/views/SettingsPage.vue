<template>
  <div class="page settings-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <span class="page-title">设置</span>
      <div class="spacer"></div>
    </div>

    <div class="settings-form">
      <div class="config-grid">
        <!-- 文本 API -->
        <div class="config-section">
          <h3 class="section-title">
            <el-icon><ChatLineSquare /></el-icon> 文本处理 API
          </h3>
          <el-form label-position="top" size="large">
            <el-form-item label="Base URL">
              <el-input v-model="form.text_api_base_url" placeholder="https://api.openai.com/v1" clearable />
            </el-form-item>
            <el-form-item label="API Key">
              <el-input v-model="form.text_api_key" :placeholder="textKeyPlaceholder" type="password" show-password clearable />
              <template v-if="textApiHasKey" #error>
                <span class="key-hint">Key 已配置，输入新 Key 将覆盖旧 Key</span>
              </template>
            </el-form-item>
            <el-form-item label="Model">
              <el-input v-model="form.text_model" placeholder="gpt-4o / deepseek-chat" clearable />
            </el-form-item>
          </el-form>
          <el-button type="primary" :loading="testingText" @click="testText" class="test-btn">
            <el-icon><Connection /></el-icon> 测试文本 API
          </el-button>
          <p v-if="testTextResult" :class="['test-result', testTextResult.ok ? 'success' : 'error']">
            {{ testTextResult.msg }}
          </p>
        </div>

        <!-- 多模态 API -->
        <div class="config-section">
          <h3 class="section-title">
            <el-icon><PictureFilled /></el-icon> 多模态 API <el-tag size="small" type="info" effect="dark">选填</el-tag>
          </h3>
          <el-form label-position="top" size="large">
            <el-form-item label="Base URL">
              <el-input v-model="form.multimodal_api_base_url" placeholder="https://api.openai.com/v1" clearable />
            </el-form-item>
            <el-form-item label="API Key">
              <el-input v-model="form.multimodal_api_key" :placeholder="multiKeyPlaceholder" type="password" show-password clearable />
              <template v-if="multiApiHasKey" #error>
                <span class="key-hint">Key 已配置，输入新 Key 将覆盖旧 Key</span>
              </template>
            </el-form-item>
            <el-form-item label="Model">
              <el-input v-model="form.multimodal_model" placeholder="gpt-4o / gpt-4-vision" clearable />
            </el-form-item>
          </el-form>
          <el-button type="primary" :loading="testingMultimodal" :disabled="!form.multimodal_api_base_url || (!form.multimodal_api_key && !multiApiHasKey)" @click="testMultimodal" class="test-btn">
            <el-icon><Connection /></el-icon> 测试多模态 API
          </el-button>
          <p v-if="testMultimodalResult" :class="['test-result', testMultimodalResult.ok ? 'success' : 'error']">
            {{ testMultimodalResult.msg }}
          </p>
        </div>
      </div>

      <!-- 用户昵称 -->
      <div class="config-section user-section">
        <h3 class="section-title">
          <el-icon><UserFilled /></el-icon> 用户昵称
        </h3>
        <el-form label-position="top" size="large">
          <el-form-item label="角色将以此名称称呼你">
            <el-input v-model="form.user_name" placeholder="旅人" clearable />
          </el-form-item>
        </el-form>
      </div>

      <div class="bottom-actions">
        <el-button type="primary" size="large" :loading="saving" @click="saveSettings">
          <el-icon><Check /></el-icon> 保存配置
        </el-button>
        <el-button type="primary" size="large" :loading="testingBoth" @click="testAll">
          <el-icon><Connection /></el-icon> 全部连接测试
        </el-button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ArrowLeft, UserFilled } from '@element-plus/icons-vue'
import { getConfig, saveConfig, testTextApi, testMultimodalApi } from '@/api'
import { ElMessage } from 'element-plus'

const router = useRouter()

const form = reactive({
  text_api_base_url: '',
  text_api_key: '',
  text_model: '',
  multimodal_api_base_url: '',
  multimodal_api_key: '',
  multimodal_model: '',
  user_name: ''
})

const textApiHasKey = ref(false)
const multiApiHasKey = ref(false)

// Placeholder shows masked key when configured
const textKeyPlaceholder = ref('sk-xxxxxxxx')
const multiKeyPlaceholder = ref('sk-xxxxxxxx')

const saving = ref(false)
const testingText = ref(false)
const testingMultimodal = ref(false)
const testingBoth = ref(false)
const testTextResult = ref(null)
const testMultimodalResult = ref(null)

onMounted(async () => {
  try {
    const res = await getConfig()
    if (res.code === 0 && res.data) {
      form.text_api_base_url = res.data.text_api_base_url || ''
      form.text_model = res.data.text_model || ''
      form.multimodal_api_base_url = res.data.multimodal_api_base_url || ''
      form.multimodal_model = res.data.multimodal_model || ''

      // Key fields remain empty — user sees masked placeholder, can type new key to replace
      textApiHasKey.value = res.data.text_api_has_key || false
      multiApiHasKey.value = res.data.multimodal_api_has_key || false
      textKeyPlaceholder.value = res.data.text_api_has_key
        ? 'Key 已配置: ' + (res.data.text_api_key || '********')
        : 'sk-xxxxxxxx'
      multiKeyPlaceholder.value = res.data.multimodal_api_has_key
        ? 'Key 已配置: ' + (res.data.multimodal_api_key || '********')
        : 'sk-xxxxxxxx'

      // Never populate the actual key into the input
      form.text_api_key = ''
      form.multimodal_api_key = ''
      form.user_name = res.data.user_name || '旅人'
    }
  } catch (err) {
    console.warn('Failed to load config:', err.message)
  }
})

function goBack() {
  router.push('/')
}

async function saveSettings() {
  saving.value = true
  try {
    const payload = {
      text_api_base_url: form.text_api_base_url,
      text_api_key: form.text_api_key,          // empty if unchanged → backend keeps old
      text_model: form.text_model,
      multimodal_api_base_url: form.multimodal_api_base_url,
      multimodal_api_key: form.multimodal_api_key,
      multimodal_model: form.multimodal_model,
      user_name: form.user_name
    }
    const res = await saveConfig(payload)
    if (res.code === 0) {
      ElMessage.success('Configuration saved')
      // Refresh to update hasKey state
      textApiHasKey.value = textApiHasKey.value || !!form.text_api_key
      multiApiHasKey.value = multiApiHasKey.value || !!form.multimodal_api_key
      form.text_api_key = ''
      form.multimodal_api_key = ''
      // Reload placeholders
      const fresh = await getConfig()
      if (fresh.code === 0 && fresh.data) {
        textApiHasKey.value = fresh.data.text_api_has_key
        multiApiHasKey.value = fresh.data.multimodal_api_has_key
        textKeyPlaceholder.value = fresh.data.text_api_has_key
          ? 'Key 已配置: ' + fresh.data.text_api_key
          : 'sk-xxxxxxxx'
        multiKeyPlaceholder.value = fresh.data.multimodal_api_has_key
          ? 'Key 已配置: ' + fresh.data.multimodal_api_key
          : 'sk-xxxxxxxx'
      }
    } else {
      ElMessage.error(res.message || 'Save failed')
    }
  } catch (err) {
    ElMessage.error('Save failed: ' + err.message)
  } finally {
    saving.value = false
  }
}

async function testText() {
  testingText.value = true
  testTextResult.value = null
  try {
    // Save current form first, respecting key logic
    await saveConfig({
      text_api_base_url: form.text_api_base_url,
      text_api_key: form.text_api_key,
      text_model: form.text_model
    })
    const res = await testTextApi()
    testTextResult.value = { ok: res.code === 0, msg: res.message }
  } catch (err) {
    testTextResult.value = { ok: false, msg: 'Connection failed: ' + err.message }
  } finally {
    testingText.value = false
  }
}

async function testMultimodal() {
  testingMultimodal.value = true
  testMultimodalResult.value = null
  try {
    await saveConfig({
      multimodal_api_base_url: form.multimodal_api_base_url,
      multimodal_api_key: form.multimodal_api_key,
      multimodal_model: form.multimodal_model
    })
    const res = await testMultimodalApi()
    testMultimodalResult.value = { ok: res.code === 0, msg: res.message }
  } catch (err) {
    testMultimodalResult.value = { ok: false, msg: 'Connection failed: ' + err.message }
  } finally {
    testingMultimodal.value = false
  }
}

async function testAll() {
  testingBoth.value = true
  await Promise.all([testText(), testMultimodal()])
  testingBoth.value = false
}
</script>

<style scoped>
.settings-page { padding-bottom: 80px; }

.top-bar { display: flex; align-items: center; gap: 14px; padding: 8px 0 32px; }
.back-btn {
  background: var(--ink-700) !important; border-color: var(--ink-500) !important; color: var(--star-soft) !important;
}
.back-btn:hover { border-color: var(--gold-dim) !important; color: var(--gold) !important; }
.page-title {
  font-family: var(--font-serif); font-size: var(--fs-xl); font-weight: 600;
  color: var(--star); letter-spacing: 0.02em;
}
.spacer { flex: 1; }

.settings-form { max-width: 1000px; margin: 0 auto; }

.config-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 24px; margin-bottom: 24px; }

.config-section {
  background: var(--ink-700); border: 1px solid var(--ink-500);
  border-radius: var(--radius); padding: 26px 28px;
  box-shadow: var(--shadow-card);
}

.section-title {
  display: flex; align-items: center; gap: 9px;
  font-family: var(--font-serif); font-size: 17px; font-weight: 600;
  color: var(--star); margin: 0 0 22px; padding-bottom: 14px;
  border-bottom: 1px solid var(--ink-500);
}
.section-title .el-icon { color: var(--gold); }

.key-hint { font-size: 12px; color: var(--cyan); margin-left: 2px; font-family: var(--font-mono); letter-spacing: 0.02em; }

.test-btn { width: 100%; margin-top: 4px; }

.test-result {
  margin-top: 14px; padding: 10px 14px; border-radius: var(--radius-sm);
  font-size: 13px; font-family: var(--font-mono); letter-spacing: 0.02em;
}
.test-result.success { color: var(--cyan); background: rgba(94, 179, 196, 0.1); border: 1px solid rgba(94, 179, 196, 0.3); }
.test-result.error { color: var(--rust); background: rgba(199, 107, 90, 0.12); border: 1px solid rgba(199, 107, 90, 0.3); }

.bottom-actions { display: flex; justify-content: center; gap: 20px; padding: 28px 0 8px; }

@media (max-width: 768px) {
  .config-grid { grid-template-columns: 1fr; }
}
</style>
