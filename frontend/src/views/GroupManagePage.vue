<template>
  <div class="manage-page">
    <div class="top-bar">
      <el-button :icon="ArrowLeft" circle @click="goChat" />
      <span class="page-title">群聊管理</span>
    </div>

    <div class="content" v-loading="loading">
      <!-- 基本信息 -->
      <div class="section">
        <h3>基本信息</h3>
        <el-form label-position="top" size="large">
          <!-- 头像 -->
          <el-form-item label="群聊头像">
            <div class="avatar-row">
              <el-avatar :size="80" :src="avatarUrl" shape="square">
                {{ form.name?.charAt(0) }}
              </el-avatar>
              <el-upload
                :auto-upload="false"
                :show-file-list="false"
                accept="image/*"
                :on-change="onAvatarChange"
              >
                <el-button size="small">更换头像</el-button>
              </el-upload>
            </div>
          </el-form-item>

          <el-form-item label="群聊名称">
            <el-input v-model="form.name" placeholder="群聊名称" />
          </el-form-item>

          <el-form-item label="群聊说明">
            <el-input v-model="form.description" type="textarea" :rows="3" placeholder="群聊简介..." />
          </el-form-item>

          <el-form-item label="当前模式">
            <el-tag>{{ form.mode === 'auto' ? '自动对话' : '@提及' }}</el-tag>
          </el-form-item>

          <el-button type="primary" :loading="saving" @click="saveInfo">保存信息</el-button>
        </el-form>
      </div>

      <!-- 成员管理 -->
      <div class="section">
        <h3>成员管理 ({{ members.length }})</h3>
        <div class="member-list">
          <div v-for="m in members" :key="m.id" class="member-row">
            <el-avatar :size="36" :src="getAvatarUrl(m.id)">{{ m.name?.charAt(0) }}</el-avatar>
            <span class="member-name">{{ m.name }}</span>
            <el-button type="danger" size="small" circle :icon="Close" @click="removeMember(m)" />
          </div>
        </div>
        <div class="add-member">
          <el-select v-model="newMemberId" placeholder="选择要添加的角色..." filterable size="large" style="flex:1">
            <el-option v-for="c in availableChars" :key="c.id" :label="c.name" :value="c.id">
              <el-avatar :size="24" :src="getAvatarUrl(c.id)">{{ c.name?.charAt(0) }}</el-avatar>
              <span style="margin-left:8px">{{ c.name }}</span>
            </el-option>
          </el-select>
          <el-button type="primary" :disabled="!newMemberId" @click="addMember" size="large">添加</el-button>
        </div>
      </div>

      <!-- 危险区域 -->
      <div class="section danger-section">
        <h3>危险操作</h3>
        <el-button type="danger" @click="confirmDelete" size="large">
          <el-icon><Delete /></el-icon> 删除此群聊
        </el-button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, computed } from 'vue'
import { useRouter, useRoute } from 'vue-router'
import { ArrowLeft, Close, Delete } from '@element-plus/icons-vue'
import { getGroup, updateGroup, deleteGroup, addGroupMember, removeGroupMember, listCharacters, getAvatarUrl } from '@/api'
import { ElMessage, ElMessageBox } from 'element-plus'

const router = useRouter()
const route = useRoute()
const gid = route.params.id

const loading = ref(true)
const saving = ref(false)
const members = ref([])
const allChars = ref([])
const newMemberId = ref('')
const avatarFile = ref(null)

const form = reactive({
  name: '',
  description: '',
  mode: '',
  avatar_path: ''
})

const avatarUrl = computed(() => {
  if (avatarFile.value) return URL.createObjectURL(avatarFile.value)
  if (form.avatar_path) return `/api/groups/${encodeURIComponent(gid)}/avatar`
  return ''
})

const memberIds = computed(() => members.value.map(m => m.id))
const availableChars = computed(() =>
  allChars.value.filter(c => !memberIds.value.includes(c.id))
)

onMounted(async () => {
  try {
    const [gr, chars] = await Promise.all([
      getGroup(gid),
      listCharacters()
    ])
    if (gr.code === 0) {
      const d = gr.data
      form.name = d.name || ''
      form.description = d.description || ''
      form.mode = d.mode || 'mention'
      form.avatar_path = d.avatar_path || ''
      members.value = d.member_details || []
    }
    if (chars.code === 0) allChars.value = chars.data || []
  } catch (e) { ElMessage.error('Failed to load') }
  finally { loading.value = false }
})

async function saveInfo() {
  if (!form.name.trim()) { ElMessage.warning('Name required'); return }
  saving.value = true
  try {
    const payload = {
      name: form.name,
      description: form.description
    }
    // If new avatar selected, convert to base64
    if (avatarFile.value) {
      const b64 = await toBase64(avatarFile.value)
      payload.avatar_base64 = b64
    }
    const res = await updateGroup(gid, payload)
    if (res.code === 0) {
      ElMessage.success('Saved')
      avatarFile.value = null
      // Refresh to get updated avatar_path
      const fresh = await getGroup(gid)
      if (fresh.code === 0) {
        form.avatar_path = fresh.data.avatar_path || ''
      }
    } else {
      ElMessage.error(res.message)
    }
  } catch (e) { ElMessage.error('Save failed') }
  finally { saving.value = false }
}

function onAvatarChange(file) {
  avatarFile.value = file.raw
}

function toBase64(file) {
  return new Promise((resolve) => {
    const reader = new FileReader()
    reader.onload = () => resolve(reader.result)
    reader.readAsDataURL(file)
  })
}

async function addMember() {
  if (!newMemberId.value) return
  try {
    const res = await addGroupMember(gid, newMemberId.value)
    if (res.code === 0) {
      ElMessage.success('Member added')
      // Refresh
      const fresh = await getGroup(gid)
      if (fresh.code === 0) members.value = fresh.data.member_details || []
      newMemberId.value = ''
    } else { ElMessage.error(res.message) }
  } catch (e) { ElMessage.error('Add failed') }
}

async function removeMember(member) {
  try {
    await ElMessageBox.confirm(`Remove ${member.name} from this group?`, 'Confirm', { type: 'warning' })
    const res = await removeGroupMember(gid, member.id)
    if (res.code === 0) {
      ElMessage.success('Member removed')
      members.value = members.value.filter(m => m.id !== member.id)
    } else { ElMessage.error(res.message) }
  } catch (e) { /* cancelled */ }
}

async function confirmDelete() {
  try {
    await ElMessageBox.confirm(
      'This will permanently delete the group and all its chat history. Continue?',
      'Delete Group', { type: 'error', confirmButtonClass: 'el-button--danger' }
    )
    const res = await deleteGroup(gid)
    if (res.code === 0) {
      ElMessage.success('Group deleted')
      router.push('/groupchats')
    } else { ElMessage.error(res.message) }
  } catch (e) { /* cancelled */ }
}

function goChat() { router.push(`/groupchats/${gid}/chat`) }
</script>

<style scoped>
.manage-page { min-height: 100vh; background: #f0f2f5; padding-bottom: 40px; }
.top-bar { display: flex; align-items: center; padding: 16px 24px; background: white; box-shadow: 0 1px 4px rgba(0,0,0,0.06); }
.page-title { margin-left: 16px; font-size: 20px; font-weight: 600; }

.content { max-width: 700px; margin: 24px auto; padding: 0 16px; }

.section { background: white; border-radius: 12px; padding: 24px; margin-bottom: 20px; box-shadow: 0 1px 6px rgba(0,0,0,0.06); }
.section h3 { margin: 0 0 20px 0; font-size: 16px; font-weight: 600; padding-bottom: 12px; border-bottom: 1px solid #ebeef5; }

.avatar-row { display: flex; align-items: center; gap: 16px; }

.member-list { display: flex; flex-direction: column; gap: 8px; margin-bottom: 16px; }
.member-row { display: flex; align-items: center; gap: 12px; padding: 8px; border-radius: 8px; background: #fafafa; }
.member-name { flex: 1; font-size: 15px; font-weight: 500; }

.add-member { display: flex; gap: 12px; align-items: center; }

.danger-section { border: 1px solid #fde2e2; }
.danger-section h3 { color: #f56c6c; border-color: #fde2e2; }
</style>
