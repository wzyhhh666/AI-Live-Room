import { io, Socket } from 'socket.io-client'

const API_BASE_URL = ''

class ApiService {
  private socket: Socket | null = null

  async login(username: string, password?: string) {
    try {
      const response = await fetch(`${API_BASE_URL}/api/auth/login`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ username }),
      })

      const data = await response.json()
      
      if (!response.ok) {
        return { success: false, error: data.error || 'Login failed' }
      }

      return { success: true, data: data.data }
    } catch (error) {
      console.error('Login API error:', error)
      return { success: false, error: 'Network error' }
    }
  }

  async getRoomInfo(roomId: string) {
    try {
      const response = await fetch(`${API_BASE_URL}/api/rooms/${roomId}`)
      const data = await response.json()
      return data
    } catch (error) {
      console.error('Get room info error:', error)
      return { success: false, error: 'Failed to get room info' }
    }
  }

  async getRecentDanmaku(roomId: string, limit: number = 50) {
    try {
      const response = await fetch(`${API_BASE_URL}/api/rooms/${roomId}/danmaku?limit=${limit}`)
      const data = await response.json()
      return data
    } catch (error) {
      console.error('Get danmaku error:', error)
      return { success: false, data: [] }
    }
  }

  async getGiftList() {
    try {
      const response = await fetch(`${API_BASE_URL}/api/gifts`)
      const data = await response.json()
      return data
    } catch (error) {
      console.error('Get gift list error:', error)
      return { success: false, error: 'Failed to get gift list' }
    }
  }

  async sendGiftHttp(roomId: string, giftId: number, count: number, userId: number, username: string) {
    try {
      const response = await fetch(`${API_BASE_URL}/api/rooms/${roomId}/gift`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ giftId, count, userId, username })
      })
      const data = await response.json()
      return data
    } catch (error) {
      console.error('Send gift error:', error)
      return { success: false, error: 'Failed to send gift' }
    }
  }

  async getGiftRank(roomId: string, limit: number = 20) {
    try {
      const response = await fetch(`${API_BASE_URL}/api/rooms/${roomId}/gift/rank?limit=${limit}`)
      const data = await response.json()
      return data
    } catch (error) {
      console.error('Get gift rank error:', error)
      return { success: false, error: 'Failed to get gift rank' }
    }
  }

  async getStreamInfo(roomId: string) {
    try {
      const response = await fetch(`${API_BASE_URL}/api/stream/rooms/${roomId}/stream/info`)
      const data = await response.json()
      return data
    } catch (error) {
      console.error('Get stream info error:', error)
      return { success: false, data: { isLiving: false, streamUrls: null } }
    }
  }

  async getRoomList(page: number = 1, pageSize: number = 20) {
    try {
      const response = await fetch(`${API_BASE_URL}/api/rooms?page=${page}&pageSize=${pageSize}`)
      const data = await response.json()
      return data
    } catch (error) {
      console.error('Get room list error:', error)
      return { success: false, data: { rooms: [], total: 0 } }
    }
  }

  async createRoom(data: { roomName: string; hostId: number; hostName: string; title: string; coverImage?: string }) {
    try {
      const response = await fetch(`${API_BASE_URL}/api/rooms`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
      })
      return await response.json()
    } catch (error) {
      console.error('Create room error:', error)
      return { success: false, error: 'Failed to create room' }
    }
  }

  async closeRoom(roomId: string, hostId: string) {
    try {
      const response = await fetch(`${API_BASE_URL}/api/rooms/${roomId}/close`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ hostId: parseInt(hostId) })
      })
      return await response.json()
    } catch (error) {
      console.error('Close room error:', error)
      return { success: false, error: 'Failed to close room' }
    }
  }

  connectSocket(): Socket {
    if (this.socket) {
      this.socket.removeAllListeners()
      this.socket.disconnect()
      this.socket = null
    }

    this.socket = io({
      transports: ['websocket', 'polling'],
      autoConnect: true,
      reconnection: true,
      reconnectionAttempts: 5,
      reconnectionDelay: 1000,
    })

    this.socket.on('connect', () => {
      console.log('✅ Socket connected:', this.socket?.id)
    })

    this.socket.on('disconnect', (reason) => {
      console.log('❌ Socket disconnected:', reason)
    })

    this.socket.on('connect_error', (error) => {
      console.error('❌ Socket connection error:', error)
    })

    return this.socket
  }

  getSocket(): Socket | null {
    return this.socket
  }

  disconnectSocket() {
    if (this.socket) {
      this.socket.disconnect()
      this.socket = null
    }
  }
}

export const apiService = new ApiService()
export default apiService
