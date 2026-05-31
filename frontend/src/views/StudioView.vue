<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useUserStore } from '@/stores/user'
import { useRoomStore } from '@/stores/room'
import { useGiftStore } from '@/stores/gift'
import apiService from '@/services/api'
import { startWhipStream, stopWhipStream } from '@/services/whipClient'
import { Socket } from 'socket.io-client'
import { 
  Send, Gift, Users, Heart, MessageCircle,
  Play, Square, LogOut
} from 'lucide-vue-next'
import GiftPanel from '@/components/gift/GiftPanel.vue'
import GiftEffect from '@/components/gift/GiftEffect.vue'
import GiftRank from '@/components/gift/GiftRank.vue'
import DanmakuCanvas from '@/components/player/DanmakuCanvas.vue'

const route = useRoute()
const router = useRouter()
const userStore = useUserStore()
const roomStore = useRoomStore()
const giftStore = useGiftStore()

const roomId = route.params.roomId as string
const previewRef = ref<HTMLVideoElement | null>(null)
const streamKey = ref('')
const flvUrl = ref('')
const hlsUrl = ref('')
const isStreaming = ref(false)
const isStarting = ref(false)
const error = ref('')

const newDanmaku = ref('')
const panelMode = ref<'chat' | 'gift' | 'rank'>('chat')
const sendingDanmaku = ref(false)
let lastDanmakuTime = 0

let pc: RTCPeerConnection | null = null
let mediaStream: MediaStream | null = null
let socket: Socket | null = null

const roomTitle = ref('')
const onlineCount = ref(0)
const likeCount = ref(0)

onMounted(async () => {
  try {
    const roomResult = await apiService.getRoomInfo(roomId)
    if (roomResult.success) {
      const roomData = roomResult.data
      roomTitle.value = roomData.title || 'AI Live Room'
      onlineCount.value = roomData.onlineCount || 0
      likeCount.value = roomData.likeCount || 0
      roomStore.joinRoom(roomData)
    }

    socket = apiService.connectSocket()

    socket.on('joined-room', (data) => {
      onlineCount.value = data.onlineUsers || onlineCount.value
    })

    socket.on('new-danmaku', (message) => {
      roomStore.addDanmaku({
        id: message.id,
        userId: message.userId,
        username: message.username,
        content: message.content,
        color: message.color || '#00ff41',
        time: message.time || '',
        type: message.type || 'normal'
      })
    })

    socket.on('online-count', (data) => {
      onlineCount.value = data.count
    })

    socket.on('stats-update', (data) => {
      if (data.likeCount !== undefined) {
        likeCount.value = data.likeCount
      } else {
        likeCount.value += data.likeDelta || 0
      }
    })

    socket.on('new-gift', (data) => {
      const giftItem = giftStore.giftList.find(g => g.id === data.giftId)
      giftStore.triggerEffect({
        id: data.giftId || 0,
        giftName: data.giftName || '礼物',
        giftIcon: giftItem?.giftIcon || '🎁',
        count: data.count || 1,
        totalPrice: data.totalPrice || 0,
        senderName: data.senderName || 'Unknown',
        effectType: data.effectType || 'normal',
        timestamp: data.timestamp || Date.now()
      })
    })

    socket.on('gift-rank-update', (rankData) => {
      // #region debug-point D:rcv-rank
      fetch("http://127.0.0.1:7778/event",{method:"POST",body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"D",location:"StudioView.vue:100",msg:"[DEBUG] StudioView received gift-rank-update",data:{length:rankData?.length||0,data:rankData}})}).catch(()=>{});
      // #endregion
      giftStore.updateRank(rankData)
    })

    socket.emit('join-room', {
      roomId,
      userInfo: userStore.userInfo
    })

    await giftStore.fetchGiftList()

    startStream()
  } catch (e) {
    console.error('[StudioView] init error:', e)
  }
})

async function startStream() {
  if (isStarting.value) return
  isStarting.value = true
  error.value = ''

  const url = `/api/stream/rooms/${roomId}/stream/start`
  console.log('[StudioView] startStream called, url:', url, 'roomId:', roomId)

  try {
    const res = await fetch(url, { method: 'POST' })
    const data = await res.json()
    console.log('[StudioView] startStream response:', data)
    if (!data.success) {
      error.value = data.error || 'Failed to start stream'
      isStarting.value = false
      return
    }

    streamKey.value = data.data.streamKey
    flvUrl.value = data.data.flvUrl
    hlsUrl.value = data.data.hlsUrl

    const result = await startWhipStream(streamKey.value)
    pc = result.pc
    mediaStream = result.mediaStream

    if (previewRef.value) {
      previewRef.value.srcObject = mediaStream
      previewRef.value.play()
    }

    isStreaming.value = true
  } catch (e: any) {
    console.error('[StudioView] startStream error:', e)
    error.value = e.message || 'Failed to start stream'
  } finally {
    isStarting.value = false
  }
}

function stopStream() {
  stopWhipStream(pc, mediaStream)
  pc = null
  mediaStream = null

  if (previewRef.value) {
    previewRef.value.srcObject = null
  }

  fetch(`/api/stream/rooms/${roomId}/stream/stop`, {
    method: 'POST'
  }).catch(() => {})

  isStreaming.value = false
}

async function endRoom() {
  stopWhipStream(pc, mediaStream)
  pc = null
  mediaStream = null

  if (previewRef.value) {
    previewRef.value.srcObject = null
  }

  try {
    if (socket && socket.connected) {
      socket.emit('send-danmaku', {
        roomId,
        content: '直播已结束',
        color: '#ff4444'
      })
    }
    await fetch(`/api/stream/rooms/${roomId}/stream/stop`, { method: 'POST' })
    await apiService.closeRoom(roomId, String(userStore.userInfo?.userId || ''))
  } catch (e) {
    console.error('endRoom error:', e)
  }

  isStreaming.value = false
  router.push('/rooms')
}

function sendDanmaku() {
  if (!newDanmaku.value.trim() || !socket) return
  if (sendingDanmaku.value) return

  const now = Date.now()
  if (now - lastDanmakuTime < 1200) return
  lastDanmakuTime = now

  sendingDanmaku.value = true
  const content = newDanmaku.value.trim()

  socket.emit('send-danmaku', {
    roomId,
    content: content
  })

  newDanmaku.value = ''
  setTimeout(() => { sendingDanmaku.value = false }, 800)
}

function sendEmoji(emoji: string) {
  if (!socket) return
  if (sendingDanmaku.value) return

  const now = Date.now()
  if (now - lastDanmakuTime < 1200) return
  lastDanmakuTime = now

  sendingDanmaku.value = true

  socket.emit('send-danmaku', {
    roomId,
    content: emoji,
    color: '#ffdd00',
    type: 'emoji'
  })

  setTimeout(() => { sendingDanmaku.value = false }, 800)
}

onUnmounted(() => {
  stopWhipStream(pc, mediaStream)
  if (socket && socket.connected) {
    socket.removeAllListeners()
    socket.disconnect()
  }
  roomStore.leaveRoom()
})
</script>

<template>
  <div class="h-screen bg-[#0F0F23] text-white flex flex-col overflow-hidden">
    <header class="flex items-center justify-between px-4 py-2 bg-black/40 backdrop-blur-sm border-b border-gray-800/50 z-20 flex-shrink-0" style="height: 40px">
      <div class="flex items-center gap-3">
        <div class="w-2.5 h-2.5 rounded-full bg-gradient-to-r from-pink-500 to-purple-500 animate-pulse" />
        <span class="text-xs font-medium text-cyan-400 tracking-wider">HOST STUDIO</span>
        <span class="text-[10px] text-gray-500">● {{ roomTitle }}</span>
      </div>

      <div class="flex items-center gap-2 text-xs text-gray-400">
        <Users class="w-3.5 h-3.5 text-cyan-400" />
        <span>{{ onlineCount.toLocaleString() }} 在线</span>
        <Heart class="w-3.5 h-3.5 text-pink-400 ml-2" />
        <span>{{ likeCount.toLocaleString() }} 点赞</span>
        <MessageCircle class="w-3.5 h-3.5 text-purple-400 ml-2" />
        <span>{{ roomStore.danmakuList.length }} 弹幕</span>
      </div>
    </header>

    <div class="flex-1 flex overflow-hidden">
      <div class="flex-1 flex flex-col min-w-0">
        <div class="relative bg-black flex-1 flex items-center justify-center overflow-hidden">
          <video
            ref="previewRef"
            class="w-full h-full object-cover"
            playsinline
            muted
          />

          <div v-if="!isStreaming && !mediaStream" class="absolute inset-0 flex items-center justify-center bg-gradient-to-br from-purple-900/30 via-blue-900/30 to-pink-900/30">
            <div class="absolute inset-0 opacity-10"
                 style="background-image: linear-gradient(rgba(131,56,236,0.3) 1px, transparent 1px), linear-gradient(90deg, rgba(131,56,236,0.3) 1px, transparent 1px); background-size: 40px 40px;" />
            <div class="relative z-10 text-center space-y-4">
              <div class="w-20 h-20 mx-auto rounded-full bg-gradient-to-br from-cyan-400/10 to-purple-500/10 flex items-center justify-center">
                <Play class="w-8 h-8 text-cyan-400/60" />
              </div>
              <p class="text-gray-400 text-sm">点击下方按钮开始直播</p>
            </div>
          </div>

          <div v-if="isStreaming" class="absolute top-4 left-4 z-10">
            <span class="flex items-center gap-1.5 px-2.5 py-1 bg-red-600/90 rounded-full text-xs font-mono">
              <span class="w-2 h-2 rounded-full bg-white animate-pulse" />
              LIVE
            </span>
          </div>

          <div v-if="isStreaming" class="absolute top-4 right-16 z-10">
            <span class="px-2.5 py-1 bg-black/60 rounded-full text-xs font-mono text-cyan-400">
              {{ onlineCount.toLocaleString() }} 观看
            </span>
          </div>

          <GiftEffect />
          <DanmakuCanvas :danmakuList="roomStore.danmakuList" />
        </div>

        <div class="bg-black/60 backdrop-blur-sm border-t border-gray-800/50 px-4 py-2.5 flex items-center justify-between flex-shrink-0">
          <div class="flex items-center gap-3">
            <button
              v-if="!isStreaming"
              @click="startStream"
              :disabled="isStarting"
              class="flex items-center gap-1.5 px-4 py-1.5 bg-red-600 hover:bg-red-500 disabled:bg-gray-700 rounded-lg text-sm font-semibold transition-colors"
            >
              <Play class="w-4 h-4" />
              {{ isStarting ? '启动中...' : '开始直播' }}
            </button>

            <button
              v-else
              @click="stopStream"
              class="flex items-center gap-1.5 px-4 py-1.5 bg-gray-700 hover:bg-gray-600 rounded-lg text-sm font-semibold transition-colors"
            >
              <Square class="w-4 h-4" />
              停止推流
            </button>

            <div v-if="error" class="text-xs text-red-400 bg-red-900/30 px-2 py-0.5 rounded">
              {{ error }}
            </div>
          </div>

          <div class="flex items-center gap-2">
            <button
              @click="endRoom"
              class="flex items-center gap-1.5 px-3 py-1.5 bg-red-700/80 hover:bg-red-700 rounded-lg text-xs font-semibold transition-colors"
            >
              <LogOut class="w-3.5 h-3.5" />
              结束直播
            </button>
          </div>
        </div>
      </div>

      <div class="w-96 bg-gray-900/80 backdrop-blur-sm border-l border-gray-800/50 flex flex-col flex-shrink-0">
        <div class="flex items-center justify-between px-4 py-3 border-b border-gray-800/50 bg-black/30">
          <div class="flex items-center gap-2">
            <button
              @click="panelMode = 'chat'"
              :class="[
                'flex items-center gap-1.5 px-2 py-1 rounded text-sm transition-colors',
                panelMode === 'chat' ? 'bg-pink-500/20 text-pink-400' : 'text-gray-400 hover:text-gray-300'
              ]"
            >
              <MessageCircle class="w-4 h-4" />
              <span>互动</span>
            </button>
            <button
              @click="panelMode = 'gift'"
              :class="[
                'flex items-center gap-1.5 px-2 py-1 rounded text-sm transition-colors',
                panelMode === 'gift' ? 'bg-pink-500/20 text-pink-400' : 'text-gray-400 hover:text-gray-300'
              ]"
            >
              <Gift class="w-4 h-4" />
              <span>礼物</span>
            </button>
            <button
              @click="panelMode = 'rank'"
              :class="[
                'flex items-center gap-1.5 px-2 py-1 rounded text-sm transition-colors',
                panelMode === 'rank' ? 'bg-pink-500/20 text-pink-400' : 'text-gray-400 hover:text-gray-300'
              ]"
            >
              <span class="text-sm">🏆</span>
              <span>排行</span>
            </button>
          </div>
          <span class="text-[10px] text-gray-600">{{ roomStore.danmakuList.length }} 条消息</span>
        </div>

        <GiftPanel
          v-if="panelMode === 'gift'"
          :show="true"
          :roomId="roomId"
          :canSend="false"
          @close="panelMode = 'chat'"
        />

        <GiftRank v-else-if="panelMode === 'rank'" />

        <template v-else>
          <div class="flex-1 overflow-y-auto p-3 space-y-3 scrollbar-thin">
            <div v-if="roomStore.danmakuList.length === 0" class="flex items-center justify-center h-full text-gray-600 text-sm">
              暂无消息，等待观众互动...
            </div>
            <div v-for="msg in roomStore.recentDanmaku" :key="msg.id"
               class="group flex items-start gap-2 hover:bg-gray-800/30 p-2 rounded-lg transition-colors">
              <img src="https://api.dicebear.com/7.x/avataaars/svg?seed=default"
                   class="w-7 h-7 rounded-full flex-shrink-0 ring-1 ring-gray-700" alt="">
              <div class="flex-1 min-w-0">
                <div class="flex items-center gap-2 mb-0.5">
                  <span class="font-medium text-sm truncate" :style="{ color: msg.color }">{{ msg.username }}</span>
                  <span class="text-[10px] text-gray-600">{{ msg.time }}</span>
                </div>
                <p class="text-sm text-gray-300 break-words">{{ msg.content }}</p>
              </div>
            </div>
          </div>

          <div class="p-3 border-t border-gray-800/50 bg-black/30">
            <form @submit.prevent="sendDanmaku" class="flex gap-2">
              <input
                v-model="newDanmaku"
                type="text"
                placeholder="和观众互动..."
                :disabled="sendingDanmaku"
                class="flex-1 px-3 py-2 bg-gray-800/60 border border-gray-700 rounded-lg text-sm text-white placeholder-gray-500 focus:outline-none focus:border-cyan-500 focus:ring-1 focus:ring-cyan-500/20 transition-all disabled:opacity-50"
              >
              <button type="submit" :disabled="sendingDanmaku" class="px-3 py-2 bg-gradient-to-r from-cyan-500 to-blue-500 rounded-lg font-medium text-sm hover:shadow-lg hover:shadow-cyan-500/30 transition-all active:scale-95 disabled:opacity-50">
                <Send class="w-4 h-4" />
              </button>
            </form>

            <div class="flex gap-2 mt-2 text-lg">
              <button @click="sendEmoji('😀')" class="hover:scale-125 transition-transform">😀</button>
              <button @click="sendEmoji('❤️')" class="hover:scale-125 transition-transform">❤️</button>
              <button @click="sendEmoji('👍')" class="hover:scale-125 transition-transform">👍</button>
              <button @click="sendEmoji('🎉')" class="hover:scale-125 transition-transform">🎉</button>
              <button @click="sendEmoji('🔥')" class="hover:scale-125 transition-transform">🔥</button>
              <button @click="sendEmoji('✨')" class="hover:scale-125 transition-transform">✨</button>
            </div>
          </div>
        </template>
      </div>
    </div>
  </div>
</template>

<style scoped>
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
</style>
