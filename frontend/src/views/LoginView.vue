<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useUserStore } from '@/stores/user'
import { Zap, Wifi, Shield } from 'lucide-vue-next'

const router = useRouter()
const userStore = useUserStore()

const username = ref('')
const isLoading = ref(false)
const errorMessage = ref('')

const isConnecting = ref(true)
const connectionStatus = ref('正在连接服务器...')

onMounted(async () => {
  try {
    const response = await fetch('/api/health')
    const data = await response.json()
    isConnecting.value = false
    connectionStatus.value = data.status === 'ok' ? '就绪' : '服务器异常'
  } catch {
    isConnecting.value = false
    connectionStatus.value = '离线'
  }
})

async function handleLogin() {
  if (!username.value.trim()) {
    errorMessage.value = '请输入你的名字'
    return
  }
  
  isLoading.value = true
  errorMessage.value = ''
  
  const result = await userStore.login(username.value.trim())
  
  if (result.success) {
    router.push('/rooms')
  } else {
    errorMessage.value = result.error || '登录失败，请重试'
    isLoading.value = false
  }
}
</script>

<template>
  <div class="min-h-screen flex items-center justify-center overflow-hidden relative" style="background-color: var(--bg-primary)">
    <div class="absolute inset-0 bg-[linear-gradient(rgba(18,16,41,0.95),rgba(18,16,41,0.95)),repeating-linear-gradient(0deg,transparent,transparent_2px,rgba(131,56,236,0.03)_2px,transparent_4px),repeating-linear-gradient(90deg,transparent,transparent_2px,rgba(131,56,236,0.03)_2px,transparent_4px)]"></div>
    
    <div class="absolute top-1/4 left-1/4 w-96 h-96 bg-purple-600/20 rounded-full blur-3xl animate-pulse"></div>
    <div class="absolute bottom-1/4 right-1/4 w-96 h-96 bg-blue-600/20 rounded-full blur-3xl animate-pulse" style="animation-delay: 1s"></div>
    
    <div class="relative z-10 w-full max-w-md mx-4">
      <div class="text-center mb-8">
        <div class="inline-flex items-center gap-3 mb-4">
          <Zap class="w-12 h-12 text-cyan-400 animate-pulse" />
          <h1 class="text-5xl font-bold bg-gradient-to-r from-pink-500 via-purple-500 to-cyan-500 bg-clip-text text-transparent tracking-wider"
              style="font-family: 'Orbitron', sans-serif;">
            AI LIVE
          </h1>
        </div>
        <p class="text-gray-400 text-lg tracking-wide">NEXT GENERATION STREAMING PLATFORM</p>
      </div>
      
      <div class="backdrop-blur-xl bg-gray-900/60 border border-purple-500/30 rounded-3xl p-8 shadow-2xl shadow-purple-900/20 relative overflow-hidden">
        <div class="absolute top-0 left-0 right-0 h-1 bg-gradient-to-r from-pink-500 via-purple-500 to-cyan-500"></div>
        
        <div class="flex items-center gap-2 mb-6 text-sm">
          <Wifi :class="['w-4 h-4', isConnecting ? 'text-yellow-400 animate-pulse' : connectionStatus === '离线' ? 'text-red-400' : 'text-green-400']" />
          <span :class="[isConnecting ? 'text-yellow-400' : connectionStatus === '离线' ? 'text-red-400' : 'text-green-400']">{{ connectionStatus }}</span>
          <Shield v-if="!isConnecting" class="w-4 h-4 ml-auto text-cyan-400" />
        </div>
        
        <form @submit.prevent="handleLogin" class="space-y-6">
          <div class="space-y-2">
            <label class="block text-sm font-medium text-gray-300 uppercase tracking-wider">你的名字</label>
            <input 
              v-model="username"
              type="text" 
              placeholder="取个名字开始直播..."
              class="w-full px-4 py-4 bg-black/40 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-cyan-500 focus:ring-2 focus:ring-cyan-500/20 transition-all duration-300 text-center text-lg"
              :disabled="isLoading"
              autofocus
              maxlength="20"
            >
          </div>
          
          <div v-if="errorMessage" class="p-3 bg-red-500/10 border border-red-500/50 rounded-lg text-red-400 text-sm text-center">
            ⚠️ {{ errorMessage }}
          </div>
          
          <button 
            type="submit"
            :disabled="isLoading || isConnecting || !username.trim()"
            class="w-full relative group overflow-hidden px-6 py-4 bg-gradient-to-r from-purple-600 via-pink-600 to-red-600 rounded-xl font-semibold text-white text-lg tracking-widest transition-all duration-300 hover:shadow-lg hover:shadow-purple-500/50 disabled:opacity-50 disabled:cursor-not-allowed"
          >
            <span class="relative z-10 flex items-center justify-center gap-2">
              <template v-if="isLoading">
                <svg class="animate-spin h-5 w-5" viewBox="0 0 24 24">
                  <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4" fill="none"></circle>
                  <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z"></path>
                </svg>
                正在进入...
              </template>
              <template v-else>
                <Zap class="w-5 h-5" />
                进入直播间
              </template>
            </span>
            
            <div class="absolute inset-0 bg-gradient-to-r from-transparent via-white/20 to-transparent translate-x-[-100%] group-hover:translate-x-[100%] transition-transform duration-700 skew-x-12"></div>
          </button>
        </form>
        
        <div class="mt-8 pt-6 border-t border-gray-800 text-center">
          <p class="text-xs text-gray-500 tracking-wider">
            AI-POWERED <span class="text-cyan-400">LIVE</span> STREAMING •
            <span class="text-purple-400">REAL-TIME</span> INTERACTION
          </p>
        </div>
      </div>
      
      <div class="mt-6 text-center text-xs text-gray-600">
        v1.0.0 • AI Live Platform
      </div>
    </div>
  </div>
</template>

<style scoped>
@import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&display=swap');

input::placeholder {
  color: #6b7280;
}
</style>
