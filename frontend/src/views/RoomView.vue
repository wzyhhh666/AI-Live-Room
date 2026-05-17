<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue'
import { useRoute } from 'vue-router'
import { useUserStore } from '@/stores/user'
import { useRoomStore } from '@/stores/room'
import apiService from '@/services/api'
import { io, Socket } from 'socket.io-client'
import { 
  Send, Gift, Users, Settings, Minus, Square, Maximize2, X,
  Heart, MessageCircle, Share2, Volume2, VolumeX, Mic, VideoOff
} from 'lucide-vue-next'

const route = useRoute()
const userStore = useUserStore()
const roomStore = useRoomStore()

const newDanmaku = ref('')
const isMuted = ref(false)
const isFullscreen = ref(false)
const showGiftPanel = ref(false)
const showUserList = ref(true)

const roomId = computed(() => route.params.roomId as string || '1')

let socket: Socket | null = null

const hostInfo = ref({
  name: '',
  avatar: '',
  followers: '0',
  title: ''
})

const onlineCount = ref(0)
const likeCount = ref(0)

const danmakuList = ref<Array<{
  id: number
  username: string
  content: string
  color: string
  time: string
}>>([])

onMounted(async () => {
  try {
    console.log('🚀 Initializing room view...')
    
    const roomResult = await apiService.getRoomInfo(roomId.value)
    if (roomResult.success) {
      const roomData = roomResult.data
      
      hostInfo.value = {
        name: roomData.hostName || 'AI主播',
        avatar: roomData.coverImage || 'https://api.dicebear.com/7.x/avataaars/svg?seed=host',
        followers: `${(roomData.onlineCount * 10).toFixed(1)}K`,
        title: roomData.title || 'AI Live Room'
      }
      
      onlineCount.value = roomData.onlineCount || 0
      likeCount.value = Math.floor(Math.random() * 50000) + 10000
      
      roomStore.joinRoom(roomData)
    }
    
    const danmakuResult = await apiService.getRecentDanmaku(roomId.value, 20)
    if (danmakuResult.success && danmakuResult.data) {
      danmakuList.value = danmakuResult.data
    }
    
    socket = apiService.connectSocket()
    
    socket.emit('join-room', {
      roomId: roomId.value,
      userInfo: userStore.userInfo
    })
    
    socket.on('joined-room', (data) => {
      console.log('✅ Successfully joined room:', data)
      onlineCount.value = data.onlineUsers || onlineCount.value
    })
    
    socket.on('room-info', (data) => {
      console.log('📺 Room info received:', data)
      hostInfo.value.name = data.hostName || hostInfo.value.name
      hostInfo.value.title = data.title || hostInfo.value.title
    })
    
    socket.on('recent-danmaku', (messages) => {
      console.log(`📝 Received ${messages.length} recent messages`)
      danmakuList.value = messages
    })
    
    socket.on('new-danmaku', (message) => {
      console.log('💬 New danmaku received:', message.username, ':', message.content)
      
      danmakuList.value.push({
        id: message.id,
        username: message.username,
        content: message.content,
        color: message.color || '#00ff41',
        time: message.time || new Date().toTimeString().slice(0, 8)
      })
      
      if (danmakuList.value.length > 50) {
        danmakuList.value.shift()
      }
    })
    
    socket.on('online-count', (data) => {
      console.log('👥 Online count updated:', data.count)
      onlineCount.value = data.count
    })
    
    socket.on('stats-update', (data) => {
      likeCount.value += data.likeDelta || 0
    })
    
    socket.on('user-typing', (data) => {
      console.log('⌨️ User typing:', data.username)
    })
    
    socket.on('error', (error) => {
      console.error('❌ Socket error:', error.message)
    })

    console.log('🎉 Room view initialized successfully!')
    
  } catch (error) {
    console.error('❌ Failed to initialize room:', error)
  }
})

onUnmounted(() => {
  if (socket && socket.connected) {
    socket.disconnect()
  }
})

function sendDanmaku() {
  if (!newDanmaku.value.trim() || !socket) return
  
  const content = newDanmaku.value.trim()
  
  socket.emit('send-danmaku', {
    roomId: roomId.value,
    content: content
  })
  
  const now = new Date()
  const timeStr = `${String(now.getHours()).padStart(2,'0')}:${String(now.getMinutes()).padStart(2,'0')}:${String(now.getSeconds()).padStart(2,'0')}`
  
  danmakuList.value.push({
    id: Date.now(),
    username: userStore.displayName,
    content: content,
    color: '#00ff41',
    time: timeStr
  })
  
  if (danmakuList.value.length > 50) {
    danmakuList.value.shift()
  }
  
  newDanmaku.value = ''
}

function toggleMute() {
  isMuted.value = !isMuted.value
}

function toggleFullscreen() {
  isFullscreen.value = !isFullscreen.value
  if (!document.fullscreenElement) {
    document.documentElement.requestFullscreen()
  } else {
    document.exitFullscreen()
  }
}

function handleWindowAction(action: string) {
  window.electronAPI?.windowControl[action as keyof typeof window.electronAPI.windowControl]()
}
</script>

<template>
  <div class="h-screen bg-[#0F0F23] text-white flex flex-col overflow-hidden relative">
    <!-- 自定义标题栏 -->
    <div class="flex items-center justify-between px-4 py-2 bg-black/40 backdrop-blur-sm border-b border-gray-800/50 z-20" style="height: 32px; -webkit-app-region: drag;">
      <div class="flex items-center gap-2" style="-webkit-app-region: no-drag;">
        <div class="w-3 h-3 rounded-full bg-gradient-to-r from-pink-500 to-purple-500 animate-pulse"></div>
        <span class="text-xs font-medium text-cyan-400 tracking-wider">AI LIVE ROOM</span>
        <span class="text-[10px] text-gray-500">● LIVE</span>
      </div>
      
      <div class="text-xs text-gray-400 font-mono">{{ hostInfo.title }}</div>
      
      <!-- 窗口控制按钮 -->
      <div class="flex items-center gap-1" style="-webkit-app-region: no-drag;">
        <button @click="handleWindowAction('minimize')" class="p-1 hover:bg-gray-700/50 rounded transition-colors">
          <Minus class="w-3 h-3" />
        </button>
        <button @click="handleWindowAction('maximize')" class="p-1 hover:bg-gray-700/50 rounded transition-colors">
          <Maximize2 class="w-3 h-3" />
        </button>
        <button @click="handleWindowAction('close')" class="p-1 hover:bg-red-500/80 rounded transition-colors group">
          <X class="w-3 h-3 group-hover:text-white" />
        </button>
      </div>
    </div>

    <!-- 主内容区域 -->
    <div class="flex-1 flex overflow-hidden">
      <!-- 左侧：视频+弹幕区域 -->
      <div class="flex-1 flex flex-col min-w-0">
        <!-- 视频播放器区域 -->
        <div class="relative bg-black flex-1 flex items-center justify-center overflow-hidden">
          <!-- 视频占位符 (16:9) -->
          <div class="relative w-full h-full max-h-[calc(100vh-180px)] bg-gradient-to-br from-purple-900/30 via-blue-900/30 to-pink-900/30 flex items-center justify-center">
            <!-- 网格背景效果 -->
            <div class="absolute inset-0 opacity-10"
                 style="background-image: linear-gradient(rgba(131,56,236,0.3) 1px, transparent 1px), linear-gradient(90deg, rgba(131,56,236,0.3) 1px, transparent 1px); background-size: 40px 40px;"></div>
            
            <!-- 主播信息覆盖层 -->
            <div class="absolute top-6 left-6 z-10">
              <div class="flex items-center gap-3 backdrop-blur-md bg-black/60 border border-purple-500/30 rounded-xl p-3">
                <img :src="hostInfo.avatar" class="w-12 h-12 rounded-full ring-2 ring-cyan-400" alt="avatar">
                <div>
                  <div class="font-bold text-lg bg-gradient-to-r from-pink-400 to-purple-400 bg-clip-text text-transparent">{{ hostInfo.name }}</div>
                  <div class="text-xs text-gray-400">{{ hostInfo.followers }} followers</div>
                </div>
              </div>
            </div>
            
            <!-- 直播状态标签 -->
            <div class="absolute top-6 right-6 z-10">
              <div class="flex items-center gap-2 backdrop-blur-md bg-red-600/90 border border-red-500/50 rounded-full px-4 py-2 animate-pulse">
                <div class="w-2 h-2 bg-white rounded-full"></div>
                <span class="text-sm font-bold uppercase tracking-wider">LIVE</span>
                <span class="text-xs opacity-80">{{ onlineCount.toLocaleString() }} watching</span>
              </div>
            </div>
            
            <!-- 中央播放提示 -->
            <div class="text-center z-10">
              <div class="inline-flex items-center justify-center w-24 h-24 rounded-full bg-gradient-to-br from-purple-600 to-pink-600 mb-4 shadow-2xl shadow-purple-500/50">
                <VideoOff class="w-12 h-12" />
              </div>
              <p class="text-xl font-bold mb-2">Video Player Ready</p>
              <p class="text-sm text-gray-400">Connect to stream source to begin playback</p>
              <p class="text-xs text-cyan-400 mt-2">🔌 Connected to API Server • Real-time Data</p>
            </div>
            
            <!-- 弹幕叠加层 -->
            <div class="absolute bottom-0 left-0 right-0 h-48 pointer-events-none overflow-hidden">
              <div v-for="(msg, index) in danmakuList.slice(-8)" :key="msg.id"
                   class="absolute whitespace-nowrap animate-slide-in font-medium text-shadow-lg"
                   :style="{ 
                     bottom: `${(index % 6) * 28}px`,
                     left: '100%',
                     color: msg.color,
                     animationDuration: `${8 + Math.random() * 4}s`,
                     animationDelay: `${index * 0.1}s`
                   }">
                <span class="text-xs opacity-70 mr-2">{{ msg.username }}:</span>{{ msg.content }}
              </div>
            </div>
          </div>
          
          <!-- 视频控制栏 -->
          <div class="absolute bottom-0 left-0 right-0 bg-gradient-to-t from-black/90 via-black/60 to-transparent py-3 px-6 flex items-center gap-4">
            <button class="hover:text-cyan-400 transition-colors">
              <Heart class="w-5 h-5" />
            </button>
            <span class="text-sm text-gray-300">{{ likeCount.toLocaleString() }}</span>
            
            <div class="flex-1"></div>
            
            <button @click="toggleMute" class="hover:text-cyan-400 transition-colors">
              <VolumeX v-if="isMuted" class="w-5 h-5" />
              <Volume2 v-else class="w-5 h-5" />
            </button>
            
            <button @click="toggleFullscreen" class="hover:text-cyan-400 transition-colors ml-2">
              <Maximize2 class="w-5 h-5" />
            </button>
          </div>
        </div>
        
        <!-- 底部信息栏 -->
        <div class="bg-black/60 backdrop-blur-sm border-t border-gray-800/50 px-6 py-3 flex items-center justify-between">
          <div class="flex items-center gap-4">
            <Users class="w-4 h-4 text-cyan-400" />
            <span class="text-sm">{{ onlineCount.toLocaleString() }}</span>
            <MessageCircle class="w-4 h-4 text-pink-400 ml-4" />
            <span class="text-sm">{{ danmakuList.length }}</span>
          </div>
          
          <div class="flex items-center gap-2">
            <Share2 class="w-4 h-4 text-gray-400 hover:text-cyan-400 cursor-pointer" />
            <Settings class="w-4 h-4 text-gray-400 hover:text-cyan-400 cursor-pointer" />
          </div>
        </div>
      </div>
      
      <!-- 右侧：聊天+礼物面板 -->
      <div class="w-96 bg-gray-900/80 backdrop-blur-sm border-l border-gray-800/50 flex flex-col">
        <!-- 面板头部 -->
        <div class="flex items-center justify-between px-4 py-3 border-b border-gray-800/50 bg-black/30">
          <div class="flex items-center gap-2">
            <MessageCircle class="w-4 h-4 text-pink-400" />
            <span class="font-semibold">Live Chat</span>
            <span class="text-xs text-gray-500">({{ danmakuList.length }})</span>
          </div>
          <div class="flex items-center gap-2">
            <button 
              @click="showUserList = !showUserList"
              :class="['p-1.5 rounded transition-colors', showUserList ? 'bg-cyan-500/20 text-cyan-400' : 'hover:bg-gray-800']"
            >
              <Users class="w-4 h-4" />
            </button>
            <button 
              @click="showGiftPanel = !showGiftPanel"
              :class="['p-1.5 rounded transition-colors', showGiftPanel ? 'bg-pink-500/20 text-pink-400' : 'hover:bg-gray-800']"
            >
              <Gift class="w-4 h-4" />
            </button>
          </div>
        </div>
        
        <!-- 聊天消息列表 -->
        <div class="flex-1 overflow-y-auto p-4 space-y-3 scrollbar-thin">
          <div v-for="msg in danmakuList.slice().reverse()" :key="msg.id" 
               class="group flex items-start gap-2 hover:bg-gray-800/30 p-2 rounded-lg transition-colors">
            <img src="https://api.dicebear.com/7.x/avataaars/svg?seed=default" 
                 class="w-8 h-8 rounded-full flex-shrink-0 ring-1 ring-gray-700" alt="">
            <div class="flex-1 min-w-0">
              <div class="flex items-center gap-2 mb-1">
                <span class="font-medium text-sm truncate" :style="{ color: msg.color }">{{ msg.username }}</span>
                <span class="text-[10px] text-gray-600">{{ msg.time }}</span>
              </div>
              <p class="text-sm text-gray-300 break-words">{{ msg.content }}</p>
            </div>
          </div>
        </div>
        
        <!-- 输入框区域 -->
        <div class="p-4 border-t border-gray-800/50 bg-black/30">
          <form @submit.prevent="sendDanmaku" class="flex gap-2">
            <input 
              v-model="newDanmaku"
              type="text" 
              placeholder="发送弹幕..."
              class="flex-1 px-4 py-2.5 bg-gray-800/60 border border-gray-700 rounded-lg text-sm text-white placeholder-gray-500 focus:outline-none focus:border-cyan-500 focus:ring-1 focus:ring-cyan-500/20 transition-all"
            >
            <button type="submit" class="px-4 py-2.5 bg-gradient-to-r from-cyan-500 to-blue-500 rounded-lg font-medium text-sm hover:shadow-lg hover:shadow-cyan-500/30 transition-all active:scale-95">
              <Send class="w-4 h-4" />
            </button>
          </form>
          
          <!-- 快捷表情 -->
          <div class="flex gap-2 mt-2 text-lg">
            <button class="hover:scale-125 transition-transform">😀</button>
            <button class="hover:scale-125 transition-transform">❤️</button>
            <button class="hover:scale-125 transition-transform">👍</button>
            <button class="hover:scale-125 transition-transform">🎉</button>
            <button class="hover:scale-125 transition-transform">🔥</button>
            <button class="hover:scale-125 transition-transform">✨</button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
@import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700&display=translate');

/* 弹幕滑入动画 */
@keyframes slideIn {
  from {
    transform: translateX(0);
    opacity: 1;
  }
  to {
    transform: translateX(calc(-100% - 100vw));
    opacity: 0.8;
  }
}

.animate-slide-in {
  animation: slideIn linear forwards;
  text-shadow: 2px 2px 4px rgba(0,0,0,0.8), 0 0 10px currentColor;
  font-size: 14px;
  font-weight: 500;
}

/* 自定义滚动条 */
.scrollbar-thin::-webkit-scrollbar {
  width: 4px;
}

.scrollbar-thin::-webkit-scrollbar-track {
  background: transparent;
}

.scrollbar-thin::-webkit-scrollbar-thumb {
  background: linear-gradient(to bottom, #ec4899, #8b5cf6);
  border-radius: 2px;
}

.scrollbar-thin::-webkit-scrollbar-thumb:hover {
  background: linear-gradient(to bottom, #f472b6, #a78bfa);
}
</style>
