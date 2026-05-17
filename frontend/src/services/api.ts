import { io, Socket } from 'socket.io-client'

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || 'http://localhost:3000'

class ApiService {
  private socket: Socket | null = null

  async login(username: string, password?: string) {
    try {
      const response = await fetch(`${API_BASE_URL}/api/auth/login`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ username, password }),
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

  connectSocket(): Socket {
    if (this.socket?.connected) {
      return this.socket
    }

    this.socket = io(API_BASE_URL, {
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
