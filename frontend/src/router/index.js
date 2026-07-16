import { createRouter, createWebHashHistory } from 'vue-router'

const routes = [
  {
    path: '/',
    name: 'Home',
    component: () => import('@/views/HomePage.vue'),
    meta: { title: 'ChronoChat - 首页' }
  },
  {
    path: '/settings',
    name: 'Settings',
    component: () => import('@/views/SettingsPage.vue'),
    meta: { title: 'ChronoChat - 设置' }
  },
  {
    path: '/characters',
    name: 'CharacterList',
    component: () => import('@/views/CharacterListPage.vue'),
    meta: { title: 'ChronoChat - 角色列表' }
  },
  {
    path: '/characters/create',
    name: 'CharacterCreate',
    component: () => import('@/views/CharacterCreatePage.vue'),
    meta: { title: 'ChronoChat - 创建角色' }
  },
  {
    path: '/characters/:id/edit',
    name: 'CharacterEdit',
    component: () => import('@/views/CharacterEditPage.vue'),
    meta: { title: 'ChronoChat - 编辑角色' }
  },
  {
    path: '/characters/:id/chat',
    name: 'CharacterChat',
    component: () => import('@/views/CharacterChatPage.vue'),
    meta: { title: 'ChronoChat - 对话' }
  },
  {
    path: '/characters/:id/info',
    name: 'CharacterInfo',
    component: () => import('@/views/CharacterInfoPage.vue'),
    meta: { title: 'ChronoChat - 角色信息' }
  },
  // 后续页面路由将在各 Phase 中添加
]

const router = createRouter({
  history: createWebHashHistory(),
  routes,
})

// 全局路由守卫 - 设置页面标题
router.beforeEach((to) => {
  if (to.meta.title) {
    document.title = to.meta.title
  }
})

export default router
