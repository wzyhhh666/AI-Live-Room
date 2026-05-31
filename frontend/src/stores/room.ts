import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export interface RoomInfo {
  roomId: number
  roomName: string
  hostId: number
  hostName: string
  onlineCount: number
  state: string // idle, living, closed
  coverImage: string
}

export interface DanmakuMessage {
  id: number
  userId: number
  username: string
  content: string
  color: string
  timestamp: number
  type: 'normal' | 'gift' | 'system' | 'emoji'
  time?: string
}

export const useRoomStore = defineStore('room', () => {
  const currentRoom = ref<RoomInfo | null>(null)
  const danmakuList = ref<DanmakuMessage[]>([])
  const onlineUsers = ref<Map<number, string>>(new Map())
  const isConnected = ref(false)
  
  const onlineCount = computed(() => {
    return currentRoom.value?.onlineCount || onlineUsers.value.size || 0
  })
  
  const recentDanmaku = computed(() => {
    return danmakuList.value.slice(-50).reverse()
  })
  
  function joinRoom(room: RoomInfo) {
    currentRoom.value = room
    danmakuList.value = []
  }
  
  function leaveRoom() {
    currentRoom.value = null
    danmakuList.value = []
    onlineUsers.value.clear()
  }
  
  function addDanmaku(msg: Partial<DanmakuMessage> & { content: string }) {
    const ts = msg.timestamp || Date.now()
    danmakuList.value.push({
      id: msg.id || Date.now() + Math.random(),
      userId: msg.userId || 0,
      username: msg.username || 'unknown',
      content: msg.content,
      color: msg.color || '#00ff41',
      timestamp: ts,
      type: msg.type || 'normal',
      time: msg.time || `${String(new Date(ts).getHours()).padStart(2,'0')}:${String(new Date(ts).getMinutes()).padStart(2,'0')}:${String(new Date(ts).getSeconds()).padStart(2,'0')}`
    })
    
    if (danmakuList.value.length > 500) {
      danmakuList.value = danmakuList.value.slice(-300)
    }
  }
  
  function addOnlineUser(userId: number, username: string) {
    onlineUsers.value.set(userId, username)
  }
  
  function removeOnlineUser(userId: number) {
    onlineUsers.value.delete(userId)
  }
  
  function updateRoomState(state: string) {
    if (currentRoom.value) {
      currentRoom.value.state = state
    }
  }
  
  function setConnected(connected: boolean) {
    isConnected.value = connected
  }
  
  return {
    currentRoom,
    danmakuList,
    onlineUsers,
    isConnected,
    onlineCount,
    recentDanmaku,
    joinRoom,
    leaveRoom,
    addDanmaku,
    addOnlineUser,
    removeOnlineUser,
    updateRoomState,
    setConnected
  }
})
