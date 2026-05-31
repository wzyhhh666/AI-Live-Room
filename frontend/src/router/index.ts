import { createRouter, createWebHistory } from 'vue-router'
import { useUserStore } from '@/stores/user'
import LoginView from '@/views/LoginView.vue'
import RoomView from '@/views/RoomView.vue'
import RoomListView from '@/views/RoomListView.vue'
import SettingsView from '@/views/SettingsView.vue'
import StudioView from '@/views/StudioView.vue'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      name: 'login',
      component: LoginView,
      meta: { requiresAuth: false }
    },
    {
      path: '/rooms',
      name: 'roomList',
      component: RoomListView,
      meta: { requiresAuth: true }
    },
    {
      path: '/settings',
      name: 'settings',
      component: SettingsView,
      meta: { requiresAuth: true }
    },
    {
      path: '/room/:roomId?',
      name: 'room',
      component: RoomView,
      meta: { requiresAuth: false }
    },
    {
      path: '/studio/:roomId',
      name: 'studio',
      component: StudioView,
      meta: { requiresAuth: true }
    }
  ]
})

router.beforeEach((to, from, next) => {
  const userStore = useUserStore()
  
  if (to.meta.requiresAuth && !userStore.isLoggedIn) {
    next({ name: 'login' })
  } else if (to.name === 'login' && userStore.isLoggedIn) {
    next({ name: 'roomList' })
  } else {
    next()
  }
})

export default router