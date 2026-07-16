import axios from 'axios'

// 后端 API 基础地址 (开发时通过 Vite proxy 代理)
const apiClient = axios.create({
  baseURL: '/api',
  timeout: 30000,
  headers: {
    'Content-Type': 'application/json'
  }
})

// 响应拦截器 - 统一错误处理
apiClient.interceptors.response.use(
  (response) => {
    return response.data
  },
  (error) => {
    console.error('[API Error]', error.message)
    return Promise.reject(error)
  }
)

// ============================================================
// Hello API - 连通性测试
// ============================================================

/**
 * 测试后端连通性
 */
export function hello() {
  return apiClient.get('/hello')
}

/**
 * 健康检查
 */
export function ping() {
  return apiClient.get('/ping')
}

// ============================================================
// Config API - 配置管理
// ============================================================

/**
 * 获取当前配置
 */
export function getConfig() {
  return apiClient.get('/config')
}

/**
 * 保存配置
 */
export function saveConfig(config) {
  return apiClient.post('/config', config)
}

/**
 * 测试文本 API 连接
 */
export function testTextApi() {
  return apiClient.post('/config/test-text')
}

/**
 * 测试多模态 API 连接
 */
export function testMultimodalApi() {
  return apiClient.post('/config/test-multimodal')
}

// ============================================================
// Character API - 角色管理
// ============================================================

/** 获取角色列表 */
export function listCharacters() {
  return apiClient.get('/characters')
}

/** 获取单个角色详情 */
export function getCharacter(id) {
  return apiClient.get(`/characters/${encodeURIComponent(id)}`)
}

/** 创建角色 */
export function createCharacter(data) {
  return apiClient.post('/characters', data)
}

/** 更新角色 */
export function updateCharacter(id, data) {
  return apiClient.put(`/characters/${encodeURIComponent(id)}`, data)
}

/** 删除角色 */
export function deleteCharacter(id) {
  return apiClient.delete(`/characters/${encodeURIComponent(id)}`)
}

/** 获取角色头像 URL */
export function getAvatarUrl(id) {
  return `/api/characters/${encodeURIComponent(id)}/avatar`
}

/** 调用AI生成角色文件 (IDENTITY.md, SOUL.md, MEMORY.md, USER.md) */
export function generateCharacterFiles(id) {
  return apiClient.post(`/characters/${encodeURIComponent(id)}/generate`)
}

/** 测试角色专属文本API */
export function testCharacterTextApi(id) {
  return apiClient.post(`/characters/${encodeURIComponent(id)}/test-text`)
}

/** 测试角色专属多模态API */
export function testCharacterMultimodalApi(id) {
  return apiClient.post(`/characters/${encodeURIComponent(id)}/test-multimodal`)
}

/** 发送对话消息 */
export function sendMessage(id, message, mode = 'chat') {
  return apiClient.post(`/characters/${encodeURIComponent(id)}/chat`, { message, mode })
}

/** 获取对话历史 (mode: 'chat' | 'letter') */
export function getChatHistory(id, mode = 'chat') {
  return apiClient.get(`/characters/${encodeURIComponent(id)}/history`, { params: { mode } })
}

// ============================================================
// Group API - 群聊管理
// ============================================================

/** 获取群聊列表 */
export function listGroups() {
  return apiClient.get('/groups')
}

/** 获取单个群聊详情 */
export function getGroup(id) {
  return apiClient.get(`/groups/${encodeURIComponent(id)}`)
}

/** 创建群聊 */
export function createGroup(data) {
  return apiClient.post('/groups', data)
}

/** 更新群聊 */
export function updateGroup(id, data) {
  return apiClient.put(`/groups/${encodeURIComponent(id)}`, data)
}

/** 删除群聊 */
export function deleteGroup(id) {
  return apiClient.delete(`/groups/${encodeURIComponent(id)}`)
}

/** 添加群聊成员 */
export function addGroupMember(id, charId) {
  return apiClient.post(`/groups/${encodeURIComponent(id)}/members`, { char_id: charId })
}

/** 移除群聊成员 */
export function removeGroupMember(id, charId) {
  return apiClient.delete(`/groups/${encodeURIComponent(id)}/members/${encodeURIComponent(charId)}`)
}

/** 发送群聊消息 */
export function sendGroupMessage(id, message, senderId = '') {
  return apiClient.post(`/groups/${encodeURIComponent(id)}/chat`, { message, sender_id: senderId })
}

/** 获取群聊历史 */
export function getGroupHistory(id) {
  return apiClient.get(`/groups/${encodeURIComponent(id)}/history`)
}

export default apiClient
