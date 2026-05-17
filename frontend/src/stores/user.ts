import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import apiService from '@/services/api'

export interface UserInfo {
  userId: number
  username: string
  nickname: string
  avatar: string
  role: number // 0=观众, 1=主播, 2=管理员
  token: string
}

export const useUserStore = defineStore('user', () => {
  const userInfo = ref<UserInfo | null>(null)
  const isLoggedIn = ref(false)
  const token = ref<string>('')
  
  const displayName = computed(() => {
    return userInfo.value?.nickname || userInfo.value?.username || 'Guest'
  })
  
  const isHost = computed(() => {
    return userInfo.value?.role === 1
  })
  
  async function login(username: string, password?: string) {
    try {
      const result = await apiService.login(username, password)
      
      if (result.success && result.data) {
        userInfo.value = result.data as UserInfo
        
        isLoggedIn.value = true
        token.value = result.data.token || ''
        
        localStorage.setItem('userInfo', JSON.stringify(result.data))
        localStorage.setItem('token', result.data.token || '')
        
        console.log('✅ Login successful:', username)
        return { success: true }
      } else {
        console.error('❌ Login failed:', result.error)
        return { success: false, error: result.error || 'Login failed' }
      }
    } catch (error) {
      console.error('❌ Login error:', error)
      return { success: false, error: 'Network error' }
    }
  }
  
  function logout() {
    userInfo.value = null
    isLoggedIn.value = false
    token.value = ''
    
    localStorage.removeItem('userInfo')
    localStorage.removeItem('token')
    
    apiService.disconnectSocket()
  }
  
  function updateUserInfo(info: Partial<UserInfo>) {
    if (userInfo.value) {
      Object.assign(userInfo.value, info)
      localStorage.setItem('userInfo', JSON.stringify(userInfo.value))
    }
  }

  function initFromStorage() {
    try {
      const storedUserInfo = localStorage.getItem('userInfo')
      const storedToken = localStorage.getItem('token')
      
      if (storedUserInfo && storedToken) {
        userInfo.value = JSON.parse(storedUserInfo)
        isLoggedIn.value = true
        token.value = storedToken
        console.log('✅ User session restored from storage')
      }
    } catch (error) {
      console.error('❌ Failed to restore user session:', error)
      logout()
    }
  }
  
  return {
    userInfo,
    isLoggedIn,
    token,
    displayName,
    isHost,
    login,
    logout,
    updateUserInfo,
    initFromStorage
  }
})
