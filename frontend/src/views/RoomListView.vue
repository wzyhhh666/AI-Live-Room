<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import { useUserStore } from '@/stores/user'
import apiService from '@/services/api'
import RoomCard from '@/components/room/RoomCard.vue'

const router = useRouter()
const userStore = useUserStore()

const rooms = ref<any[]>([])
const loading = ref(true)
const showCreateModal = ref(false)
const newRoomName = ref('')
const newRoomTitle = ref('')
const creating = ref(false)

onMounted(async () => {
  const result = await apiService.getRoomList()
  if (result.success && result.data) {
    rooms.value = result.data.rooms
  }
  loading.value = false
})

function goToRoom(roomId: number) {
  router.push(`/room/${roomId}`)
}

async function createRoom() {
  if (!newRoomName.value.trim()) return
  creating.value = true
  try {
    const result = await apiService.createRoom({
      roomName: newRoomName.value.trim(),
      title: newRoomTitle.value.trim() || `${userStore.displayName}的直播间`,
      hostId: userStore.userInfo?.userId || 0,
      hostName: userStore.displayName
    })
    if (result.success && result.data) {
      showCreateModal.value = false
      newRoomName.value = ''
      newRoomTitle.value = ''
      router.push(`/studio/${result.data.roomId}`)
    } else {
      alert('创建失败: ' + (result.error || '未知错误'))
    }
  } catch (e: any) {
    alert('创建失败: ' + e.message)
  } finally {
    creating.value = false
  }
}
</script>

<template>
  <div class="min-h-screen" style="background-color: var(--bg-primary)">
    <div class="max-w-7xl mx-auto px-6 py-8">
      <div class="flex items-center justify-between mb-8">
        <div>
          <h1 class="text-3xl font-bold text-white" style="font-family: 'Orbitron', sans-serif;">
            🤖 AI LIVE
          </h1>
          <p class="text-gray-400 mt-1 text-sm">Discover live rooms and join the cyberpunk experience</p>
        </div>
        <div class="flex items-center gap-4">
          <span class="text-gray-400 text-sm">{{ userStore.displayName }}</span>
          <button
            @click="router.push('/settings')"
            class="px-4 py-2 rounded-lg bg-gray-800/50 text-gray-400 hover:text-white transition-colors text-sm"
          >
            ⚙ Settings
          </button>
        </div>
      </div>

      <div v-if="loading" class="flex items-center justify-center py-20">
        <div class="animate-spin rounded-full h-12 w-12 border-t-2 border-b-2 border-cyan-400"></div>
      </div>

      <div v-else>
        <div v-if="rooms.length === 0" class="text-center py-20">
          <p class="text-6xl mb-4">📡</p>
          <p class="text-gray-500 text-lg">No live rooms available</p>
          <p class="text-gray-600 text-sm mt-2">Be the first to create one!</p>
        </div>

        <div v-else class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-5">
          <RoomCard
            v-for="room in rooms"
            :key="room.roomId"
            :room-id="room.roomId"
            :room-name="room.roomName"
            :host-name="room.hostName"
            :title="room.title"
            :online-count="room.onlineCount"
            :cover-image="room.coverImage"
            :state="room.state"
            @click="goToRoom(room.roomId)"
          />
        </div>
      </div>
    </div>

    <button
      @click="showCreateModal = true"
      class="fixed bottom-8 right-8 z-40 group px-6 py-3 rounded-2xl text-sm font-medium transition-all duration-300 hover:scale-105 active:scale-95"
      style="background: linear-gradient(135deg, #1a1a2e, #16213e); border: 1px solid rgba(255, 255, 255, 0.08); box-shadow: 0 4px 24px rgba(0, 0, 0, 0.5), 0 0 0 1px rgba(255, 255, 255, 0.03) inset;"
    >
      <span class="relative z-10 bg-gradient-to-r from-gray-200 via-white to-gray-400 bg-clip-text text-transparent">
        你很好看，不信点我试试
      </span>
      <span class="absolute inset-0 rounded-2xl bg-white/5 opacity-0 group-hover:opacity-100 transition-opacity duration-300"></span>
      <span class="absolute -top-1 -right-1 w-3 h-3 bg-red-500 rounded-full animate-ping"></span>
      <span class="absolute -top-1 -right-1 w-3 h-3 bg-red-500 rounded-full"></span>
    </button>
  </div>

  <!-- 创建直播间弹窗 -->
  <div v-if="showCreateModal" class="fixed inset-0 z-50 flex items-center justify-center bg-black/60" @click.self="showCreateModal = false">
    <div class="bg-gray-900 border border-gray-800 rounded-xl p-6 w-full max-w-md mx-4">
      <h2 class="text-lg font-semibold text-white mb-4">创建直播间</h2>

      <div class="space-y-4">
        <div>
          <label class="text-sm text-gray-400 block mb-1">房间名称</label>
          <input
            v-model="newRoomName"
            placeholder="输入房间名称..."
            class="w-full px-3 py-2 bg-gray-800 border border-gray-700 rounded-lg text-white placeholder-gray-500 focus:border-cyan-400 focus:outline-none"
          />
        </div>
        <div>
          <label class="text-sm text-gray-400 block mb-1">直播标题（可选）</label>
          <input
            v-model="newRoomTitle"
            placeholder="例如: 今晚来聊天~"
            class="w-full px-3 py-2 bg-gray-800 border border-gray-700 rounded-lg text-white placeholder-gray-500 focus:border-cyan-400 focus:outline-none"
          />
        </div>
      </div>

      <div class="flex items-center gap-3 mt-6">
        <button
          @click="showCreateModal = false"
          class="flex-1 px-4 py-2 bg-gray-800 hover:bg-gray-700 text-gray-300 rounded-lg text-sm transition-colors"
        >
          取消
        </button>
        <button
          @click="createRoom"
          :disabled="creating || !newRoomName.trim()"
          class="flex-1 px-4 py-2 bg-red-600 hover:bg-red-500 disabled:bg-gray-700 text-white rounded-lg text-sm font-semibold transition-colors"
        >
          {{ creating ? '创建中...' : '创建并开播' }}
        </button>
      </div>
    </div>
  </div>
</template>
