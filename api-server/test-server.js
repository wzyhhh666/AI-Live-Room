const express = require('express')
const http = require('http')
const cors = require('cors')

const PORT = 3001

let nextUserId = 2
const users = {
  1: { id: 1, username: 'ai_host', nickname: 'AI主播', avatar: 'https://api.dicebear.com/7.x/avataaars/svg?seed=host', role: 1, token: 'host-token-001' }
}

let nextRoomId = 2
const rooms = {
  1: { id: 1, room_name: 'AI直播间', host_id: 1, host_name: 'AI主播', title: '🔥 AI技术前沿直播', cover_image: null, online_count: 0, state: 'living' }
}

let nextDanmakuId = 1
const danmaku_messages = []

let nextGiftRecordId = 1
const gift_records = []

const room_members = []

const giftConfig = [
  { id: 1, gift_name: '荧光棒', gift_icon: '🪄', price: 1.00, effect_type: 'normal', sort_order: 1 },
  { id: 2, gift_name: '点赞', gift_icon: '👍', price: 2.00, effect_type: 'normal', sort_order: 2 },
  { id: 3, gift_name: '鲜花', gift_icon: '🌸', price: 5.00, effect_type: 'rain', sort_order: 3 },
  { id: 4, gift_name: '跑车', gift_icon: '🏎️', price: 50.00, effect_type: 'rocket', sort_order: 4 },
  { id: 5, gift_name: '火箭', gift_icon: '🚀', price: 100.00, effect_type: 'explosion', sort_order: 5 },
  { id: 6, gift_name: '嘉年华', gift_icon: '🎪', price: 500.00, effect_type: 'explosion', sort_order: 6 },
]

function makeUser(user) {
  return { userId: user.id, username: user.username, nickname: user.nickname, avatar: user.avatar, role: user.role, token: user.token }
}

function makeRoom(room) {
  return { roomId: room.id, roomName: room.room_name, hostId: room.host_id, hostName: room.host_name, title: room.title, coverImage: room.cover_image, onlineCount: room.online_count, state: room.state }
}

function makeDanmaku(msg) {
  return { id: msg.id, roomId: msg.room_id, userId: msg.user_id, username: msg.username, content: msg.content, color: msg.color, type: msg.type, time: msg.time || new Date().toTimeString().slice(0, 8), timestamp: Date.now() }
}

function makeGift(item) {
  return { id: item.id, giftName: item.gift_name, giftIcon: item.gift_icon, price: parseFloat(item.price), effectType: item.effect_type, sortOrder: item.sort_order }
}

const app = express()
app.use(cors())
app.use(express.json())

app.get('/api/health', (req, res) => res.json({ success: true, status: 'ok', mode: 'in-memory-test' }))

// Auth
app.post('/api/auth/login', (req, res) => {
  const { username } = req.body
  if (!username) return res.json({ success: false, error: 'Username required' })
  
  let user = Object.values(users).find(u => u.username === username)
  if (!user) {
    const token = 'token-' + Date.now() + '-' + Math.random().toString(36).slice(2)
    const id = nextUserId++
    user = { id, username, nickname: username, avatar: `https://api.dicebear.com/7.x/avataaars/svg?seed=${encodeURIComponent(username)}`, role: 0, token }
    users[id] = user
  }
  res.json({ success: true, data: makeUser(user) })
})

// Rooms
app.get('/api/rooms', (req, res) => {
  const list = Object.values(rooms).filter(r => r.state === 'living').sort((a, b) => b.online_count - a.online_count)
  res.json({ success: true, data: { rooms: list.map(makeRoom), total: list.length, page: 1, pageSize: 20 } })
})

app.get('/api/rooms/:roomId', (req, res) => {
  const room = rooms[req.params.roomId]
  if (!room) return res.json({ success: false, error: 'Room not found' })
  res.json({ success: true, data: makeRoom(room) })
})

app.post('/api/rooms', (req, res) => {
  const { roomName, hostId, hostName, title, coverImage } = req.body
  const id = nextRoomId++
  const room = { id, room_name: roomName, host_id: hostId, host_name: hostName, title, cover_image: coverImage || null, online_count: 0, state: 'living' }
  rooms[id] = room
  res.json({ success: true, data: makeRoom(room) })
})

app.put('/api/rooms/:roomId/close', (req, res) => {
  const roomId = parseInt(req.params.roomId)
  const room = rooms[roomId]
  if (!room) return res.json({ success: false, error: 'Room not found' })
  if (room.host_id !== req.body.hostId) return res.json({ success: false, error: 'Not the host' })
  room.state = 'closed'
  res.json({ success: true })
})

app.get('/api/rooms/:roomId/members', (req, res) => {
  const roomId = parseInt(req.params.roomId)
  const list = room_members.filter(m => m.room_id === roomId && !m.leave_time)
  res.json({ success: true, data: list.map(m => ({ userId: m.user_id, username: m.username, joinTime: m.join_time })) })
})

// Danmaku
app.get('/api/rooms/:roomId/danmaku', (req, res) => {
  const roomId = parseInt(req.params.roomId)
  const limit = parseInt(req.query.limit) || 50
  const list = danmaku_messages.filter(m => m.room_id === roomId).slice(-limit)
  res.json({ success: true, data: list.map(makeDanmaku) })
})

app.post('/api/rooms/:roomId/danmaku', (req, res) => {
  const roomId = parseInt(req.params.roomId)
  const { content, userId, username, color } = req.body
  if (!content) return res.json({ success: false, error: 'Content required' })
  const id = nextDanmakuId++
  const msg = { id, room_id: roomId, user_id: userId, username: username || 'anonymous', content, color: color || '#00ff41', type: 'normal', time: new Date().toTimeString().slice(0, 8) }
  danmaku_messages.push(msg)
  res.json({ success: true, data: makeDanmaku(msg) })
})

// Gifts
app.get('/api/gifts', (req, res) => {
  res.json({ success: true, data: giftConfig.map(makeGift) })
})

app.post('/api/rooms/:roomId/gift', (req, res) => {
  const roomId = parseInt(req.params.roomId)
  const { giftId, count, userId, username } = req.body
  
  if (!giftId || !count || !userId || !username) return res.json({ success: false, error: 'Missing required fields' })
  if (count <= 0) return res.json({ success: false, error: 'Invalid count' })
  
  const gift = giftConfig.find(g => g.id === giftId)
  if (!gift) return res.json({ success: false, error: 'Gift not found' })
  
  const room = rooms[roomId]
  if (!room) return res.json({ success: false, error: 'Room not found' })
  
  const totalPrice = parseFloat(gift.price) * count
  const recordId = nextGiftRecordId++
  
  gift_records.push({
    record_id: recordId, room_id: roomId, sender_id: userId, receiver_id: room.host_id,
    gift_id: giftId, gift_count: count, total_price: totalPrice, created_at: new Date()
  })
  
  res.json({ success: true, data: {
    id: recordId, giftName: gift.gift_name, giftIcon: gift.gift_icon, count,
    totalPrice, senderName: username, effectType: gift.effect_type, roomId, timestamp: Date.now()
  }})
})

app.get('/api/rooms/:roomId/gift/rank', (req, res) => {
  res.json({ success: true, data: [] })
})

const server = http.createServer(app)
server.listen(PORT, () => {
  console.log(`🚀 Test API Server running on http://localhost:${PORT}`)
  console.log(`   Mode: IN-MEMORY (no MySQL/Redis required)`)
  console.log(`   Database: ${Object.keys(users).length} users, ${Object.keys(rooms).length} rooms, ${danmaku_messages.length} messages\n`)
})
