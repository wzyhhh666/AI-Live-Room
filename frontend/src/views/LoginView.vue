<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useUserStore } from '@/stores/user'
import { LogIn, Zap, Shield, Wifi } from 'lucide-vue-next'

const router = useRouter()
const userStore = useUserStore()

const username = ref('')
const password = ref('')
const isLoading = ref(false)
const errorMessage = ref('')
const showPassword = ref(false)

const isConnecting = ref(true)
const connectionStatus = ref('正在连接服务器...')

onMounted(async () => {
  // 模拟服务器连接检测
  await new Promise(resolve => setTimeout(resolve, 1500))
  isConnecting.value = false
  connectionStatus.value = '就绪'
})

async function handleLogin() {
  if (!username.value.trim()) {
    errorMessage.value = '请输入用户名'
    return
  }
  
  isLoading.value = true
  errorMessage.value = ''
  
  const result = await userStore.login(username.value, password.value || undefined)
  
  if (result.success) {
    router.push('/room/1')
  } else {
    errorMessage.value = result.error || '登录失败，请重试'
    isLoading.value = false
  }
}

function handleQuickLogin() {
  username.value = `User${Math.floor(Math.random() * 10000)}`
  handleLogin()
}
</script>

<template>
  <div class="min-h-screen bg-[#0F0F23] flex items-center justify-center overflow-hidden relative">
    <!-- 背景动画网格 -->
    <div class="absolute inset-0 bg-[linear-gradient(rgba(18,16,41,0.95),rgba(18,16,41,0.95)),repeating-linear-gradient(0deg,transparent,transparent_2px,rgba(131,56,236,0.03)_2px,transparent_4px),repeating-linear-gradient(90deg,transparent,transparent_2px,rgba(131,56,236,0.03)_2px,transparent_4px)]"></div>
    
    <!-- 霓虹光晕效果 -->
    <div class="absolute top-1/4 left-1/4 w-96 h-96 bg-purple-600/20 rounded-full blur-3xl animate-pulse"></div>
    <div class="absolute bottom-1/4 right-1/4 w-96 h-96 bg-blue-600/20 rounded-full blur-3xl animate-pulse" style="animation-delay: 1s"></div>
    
    <!-- 主登录卡片 -->
    <div class="relative z-10 w-full max-w-md mx-4">
      <!-- 标题区域 -->
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
      
      <!-- 登录表单卡片 -->
      <div class="backdrop-blur-xl bg-gray-900/60 border border-purple-500/30 rounded-3xl p-8 shadow-2xl shadow-purple-900/20 relative overflow-hidden">
        <!-- 卡片顶部装饰线 -->
        <div class="absolute top-0 left-0 right-0 h-1 bg-gradient-to-r from-pink-500 via-purple-500 to-cyan-500"></div>
        
        <!-- 连接状态指示器 -->
        <div class="flex items-center gap-2 mb-6 text-sm">
          <Wifi :class="['w-4 h-4', isConnecting ? 'text-yellow-400 animate-pulse' : 'text-green-400']" />
          <span :class="[isConnecting ? 'text-yellow-400' : 'text-green-400']">{{ connectionStatus }}</span>
          <Shield v-if="!isConnecting" class="w-4 h-4 ml-auto text-cyan-400" />
        </div>
        
        <!-- 表单 -->
        <form @submit.prevent="handleLogin" class="space-y-6">
          <!-- 用户名输入 -->
          <div class="space-y-2">
            <label class="block text-sm font-medium text-gray-300 uppercase tracking-wider">Username</label>
            <div class="relative group">
              <LogIn class="absolute left-4 top-1/2 -translate-y-1/2 w-5 h-5 text-gray-500 group-focus-within:text-cyan-400 transition-colors" />
              <input 
                v-model="username"
                type="text" 
                placeholder="Enter your username..."
                class="w-full pl-12 pr-4 py-4 bg-black/40 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-cyan-500 focus:ring-2 focus:ring-cyan-500/20 transition-all duration-300"
                :disabled="isLoading"
              >
            </div>
          </div>
          
          <!-- 密码输入 (可选) -->
          <div class="space-y-2">
            <label class="block text-sm font-medium text-gray-300 uppercase tracking-wider">Password <span class="text-gray-500">(optional)</span></label>
            <div class="relative group">
              <input 
                v-model="password"
                :type="showPassword ? 'text' : 'password'"
                placeholder="••••••••"
                class="w-full px-4 py-4 bg-black/40 border border-gray-700 rounded-xl text-white placeholder-gray-500 focus:outline-none focus:border-cyan-500 focus:ring-2 focus:ring-cyan-500/20 transition-all duration-300 pr-12"
                :disabled="isLoading"
              >
              <button 
                type="button"
                @click="showPassword = !showPassword"
                class="absolute right-4 top-1/2 -translate-y-1/2 text-gray-500 hover:text-cyan-400 transition-colors"
              >
                {{ showPassword ? '👁️' : '👁️‍🗨️' }}
              </button>
            </div>
          </div>
          
          <!-- 错误提示 -->
          <div v-if="errorMessage" class="p-3 bg-red-500/10 border border-red-500/50 rounded-lg text-red-400 text-sm flex items-center gap-2">
            ⚠️ {{ errorMessage }}
          </div>
          
          <!-- 登录按钮 -->
          <button 
            type="submit"
            :disabled="isLoading || isConnecting"
            class="w-full relative group overflow-hidden px-6 py-4 bg-gradient-to-r from-purple-600 via-pink-600 to-red-600 rounded-xl font-semibold text-white uppercase tracking-widest transition-all duration-300 hover:shadow-lg hover:shadow-purple-500/50 disabled:opacity-50 disabled:cursor-not-allowed"
          >
            <span class="relative z-10 flex items-center justify-center gap-2">
              <template v-if="isLoading">
                <svg class="animate-spin h-5 w-5" viewBox="0 0 24 24">
                  <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4" fill="none"></circle>
                  <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4z"></path>
                </svg>
                Connecting...
              </template>
              <template v-else>
                <Zap class="w-5 h-5" />
                Enter the Matrix
              </template>
            </span>
            
            <!-- 悬停光效 -->
            <div class="absolute inset-0 bg-gradient-to-r from-transparent via-white/20 to-transparent translate-x-[-100%] group-hover:translate-x-[100%] transition-transform duration-700 skew-x-12"></div>
          </button>
          
          <!-- 快速登录按钮 -->
          <button 
            type="button"
            @click="handleQuickLogin"
            :disabled="isConnecting"
            class="w-full px-6 py-3 bg-transparent border border-cyan-500/50 rounded-xl text-cyan-400 font-medium hover:bg-cyan-500/10 hover:border-cyan-400 transition-all duration-300 disabled:opacity-50 disabled:cursor-not-allowed"
          >
            🚀 Quick Start (Random User)
          </button>
        </form>
        
        <!-- 底部装饰 -->
        <div class="mt-8 pt-6 border-t border-gray-800 text-center">
          <p class="text-xs text-gray-500 tracking-wider">
            SECURED BY <span class="text-cyan-400">AES-256</span> ENCRYPTION • 
            <span class="text-purple-400">END-TO-END</span> PROTECTED
          </p>
        </div>
      </div>
      
      <!-- 版本信息 -->
      <div class="mt-6 text-center text-xs text-gray-600">
        v1.0.0 • Electron + Vue 3 • Cyberpunk Edition
      </div>
    </div>
  </div>
</template>

<style scoped>
@import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&display=swap');

input::placeholder {
  color: #6b7280;
}

/* 自定义滚动条 */
::-webkit-scrollbar {
  width: 6px;
}

::-webkit-scrollbar-track {
  background: #1f2937;
}

::-webkit-scrollbar-thumb {
  background: linear-gradient(to bottom, #ec4899, #8b5cf6);
  border-radius: 3px;
}
</style>
