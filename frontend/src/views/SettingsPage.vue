<template>
  <div class="settings-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle class="back-btn" @click="goBack" />
      <span class="page-title">设置</span>
      <div class="spacer"></div>
    </div>

    <div class="settings-form">
      <div class="config-grid">
        <!-- Left: Text API -->
        <div class="config-section">
          <h3 class="section-title">
            <el-icon><ChatLineSquare /></el-icon> 文本处理 API
          </h3>
          <el-form label-position="top" size="large">
            <el-form-item label="Base URL">
              <el-input v-model="form.text_api_base_url" placeholder="https://api.openai.com/v1" clearable />
            </el-form-item>
            <el-form-item label="API Key">
              <!-- Show masked key as placeholder; user types new key to replace -->
              <el-input
                v-model="form.text_api_key"
                :placeholder="textKeyPlaceholder"
                type="password"
                show-password
                clearable
              />
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

        <!-- Right: Multimodal API -->
        <div class="config-section">
          <h3 class="section-title">
            <el-icon><PictureFilled /></el-icon> 多模态 API <el-tag size="small" type="info">选填</el-tag>
          </h3>
          <el-form label-position="top" size="large">
            <el-form-item label="Base URL">
              <el-input v-model="form.multimodal_api_base_url" placeholder="https://api.openai.com/v1" clearable />
            </el-form-item>
            <el-form-item label="API Key">
              <el-input
                v-model="form.multimodal_api_key"
                :placeholder="multiKeyPlaceholder"
                type="password"
                show-password
                clearable
              />
              <template v-if="multiApiHasKey" #error>
                <span class="key-hint">Key 已配置，输入新 Key 将覆盖旧 Key</span>
              </template>
            </el-form-item>
            <el-form-item label="Model">
              <el-input v-model="form.multimodal_model" placeholder="gpt-4o / gpt-4-vision" clearable />
            </el-form-item>
          </el-form>
          <el-button
            type="primary"
            :loading="testingMultimodal"
            :disabled="!form.multimodal_api_base_url || (!form.multimodal_api_key && !multiApiHasKey)"
            @click="testMultimodal"
            class="test-btn"
          >
            <el-icon><Connection /></el-icon> 测试多模态 API
          </el-button>
          <p v-if="testMultimodalResult" :class="['test-result', testMultimodalResult.ok ? 'success' : 'error']">
            {{ testMultimodalResult.msg }}
          </p>
        </div>
      </div>

      <div class="bottom-actions">
        <el-button type="success" size="large" :loading="saving" @click="saveSettings">
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
import { ArrowLeft } from '@element-plus/icons-vue'
import { getConfig, saveConfig, testTextApi, testMultimodalApi } from '@/api'
import { ElMessage } from 'element-plus'

const router = useRouter()

const form = reactive({
  text_api_base_url: '',
  text_api_key: '',
  text_model: '',
  multimodal_api_base_url: '',
  multimodal_api_key: '',
  multimodal_model: ''
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
      multimodal_model: form.multimodal_model
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
.settings-page {
  min-height: 100vh;
  background: #f0f2f5;
}

.top-bar {
  display: flex;
  align-items: center;
  padding: 16px 24px;
  background: white;
  box-shadow: 0 1px 4px rgba(0,0,0,0.06);
}

.back-btn {
  border: none;
  background: #f5f5f5;
}

.page-title {
  margin-left: 16px;
  font-size: 20px;
  font-weight: 600;
  color: #303133;
}

.spacer {
  flex: 1;
}

.settings-form {
  max-width: 1000px;
  margin: 32px auto;
  padding: 0 24px;
}

.config-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 32px;
  margin-bottom: 32px;
}

.config-section {
  background: white;
  border-radius: 12px;
  padding: 28px;
  box-shadow: 0 1px 6px rgba(0,0,0,0.06);
}

.section-title {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 17px;
  font-weight: 600;
  color: #303133;
  margin: 0 0 24px 0;
  padding-bottom: 14px;
  border-bottom: 1px solid #ebeef5;
}

.key-hint {
  font-size: 12px;
  color: #909399;
  margin-left: 4px;
}

.test-btn {
  width: 100%;
  margin-top: 4px;
}

.test-result {
  margin-top: 12px;
  padding: 8px 12px;
  border-radius: 6px;
  font-size: 13px;
}

.test-result.success {
  color: #67c23a;
  background: #f0f9eb;
}

.test-result.error {
  color: #f56c6c;
  background: #fef0f0;
}

.bottom-actions {
  display: flex;
  justify-content: center;
  gap: 20px;
  padding: 24px 0;
}

@media (max-width: 768px) {
  .config-grid {
    grid-template-columns: 1fr;
  }
}
</style>
