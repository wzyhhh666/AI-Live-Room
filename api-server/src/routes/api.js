const express = require('express')
const router = express.Router()
const roomService = require('../services/room')
const userService = require('../services/user')
const giftService = require('../services/gift')
const { enhancedHealthCheck } = require('./health')

router.get('/health', enhancedHealthCheck)
router.get('/health/live', (req, res) => res.json({ status: 'ok' }))
router.get('/health/ready', enhancedHealthCheck)

router.post('/auth/login', async (req, res) => {
  try {
    const { username, password } = req.body
    
    if (!username) {
      return res.status(400).json({
        success: false,
        error: 'Username is required'
      })
    }
    
    const result = await userService.login(username)
    res.json(result)
  } catch (error) {
    console.error('Login error:', error)
    res.status(500).json({
      success: false,
      error: 'Internal server error'
    })
  }
})

router.get('/rooms/:roomId', async (req, res) => {
  try {
    const { roomId } = req.params
    const roomInfo = await roomService.getRoomInfo(roomId)
    res.json(roomInfo)
  } catch (error) {
    console.error('Get room error:', error)
    res.status(500).json({
      success: false,
      error: 'Failed to get room info'
    })
  }
})

router.get('/rooms/:roomId/danmaku', async (req, res) => {
  try {
    const { roomId } = req.params
    const { limit = 50 } = req.query
    const danmakuList = await roomService.getRecentDanmaku(roomId, parseInt(limit))
    res.json(danmakuList)
  } catch (error) {
    console.error('Get danmaku error:', error)
    res.status(500).json({
      success: false,
      error: 'Failed to get danmaku list'
    })
  }
})

router.post('/rooms/:roomId/danmaku', async (req, res) => {
  try {
    const { roomId } = req.params
    const { content, userId, username } = req.body
    
    if (!content || !userId || !username) {
      return res.status(400).json({
        success: false,
        error: 'Missing required fields: content, userId, username'
      })
    }
    
    const result = await roomService.sendDanmaku(roomId, content, userId, username)
    res.json(result)
  } catch (error) {
    console.error('Send danmaku error:', error)
    res.status(500).json({
      success: false,
      error: 'Failed to send danmaku'
    })
  }
})

router.get('/gifts', async (req, res) => {
  try {
    const result = await giftService.getGiftList()
    res.json(result)
  } catch (error) {
    console.error('Get gifts error:', error)
    res.status(500).json({
      success: false,
      error: 'Failed to get gift list'
    })
  }
})

router.post('/rooms/:roomId/gift', async (req, res) => {
  try {
    const { roomId } = req.params
    const { giftId, count, userId, username } = req.body

    const result = await giftService.sendGift(
      parseInt(roomId),
      parseInt(giftId),
      parseInt(count) || 1,
      userId,
      username
    )
    res.json(result)
  } catch (error) {
    console.error('Send gift error:', error)
    res.status(500).json({
      success: false,
      error: 'Failed to send gift'
    })
  }
})

router.get('/rooms/:roomId/gift/rank', async (req, res) => {
  try {
    const { roomId } = req.params
    const { limit = 20 } = req.query
    const result = await giftService.getGiftRank(parseInt(roomId), parseInt(limit))
    res.json(result)
  } catch (error) {
    console.error('Get gift rank error:', error)
    res.status(500).json({
      success: false,
      error: 'Failed to get gift rank'
    })
  }
})

router.get('/rooms', async (req, res) => {
  try {
    const { page = 1, pageSize = 20 } = req.query
    const result = await roomService.getRoomList(parseInt(page), parseInt(pageSize))
    res.json(result)
  } catch (error) {
    console.error('Get room list error:', error)
    res.status(500).json({ success: false, error: 'Failed to get room list' })
  }
})

router.post('/rooms', async (req, res) => {
  try {
    const { roomName, hostId, hostName, title, coverImage } = req.body
    const result = await roomService.createRoom(roomName, hostId, hostName, title, coverImage)
    res.json(result)
  } catch (error) {
    console.error('Create room error:', error)
    res.status(500).json({ success: false, error: 'Failed to create room' })
  }
})

router.put('/rooms/:roomId/close', async (req, res) => {
  try {
    const { roomId } = req.params
    const { hostId } = req.body
    const result = await roomService.closeRoom(parseInt(roomId), hostId)

    if (result.success) {
      const io = req.app.get('io')
      if (io) {
        io.to(`room-${roomId}`).emit('room-closed', {
          roomId: parseInt(roomId),
          timestamp: Date.now()
        })
      }
    }

    res.json(result)
  } catch (error) {
    console.error('Close room error:', error)
    res.status(500).json({ success: false, error: 'Failed to close room' })
  }
})

router.get('/rooms/:roomId/members', async (req, res) => {
  try {
    const db = require('../services/database')
    const pool = await db.getMysqlPool()
    const [members] = await pool.execute(
      'SELECT user_id, username, join_time FROM room_members WHERE room_id = ? AND leave_time IS NULL',
      [parseInt(req.params.roomId)]
    )
    res.json({
      success: true,
      data: members.map(m => ({
        userId: m.user_id,
        username: m.username,
        joinTime: m.join_time
      }))
    })
  } catch (error) {
    console.error('Get members error:', error)
    res.status(500).json({ success: false, error: 'Failed to get members' })
  }
})

router.get('/rooms/:roomId/stats', async (req, res) => {
  try {
    const roomId = parseInt(req.params.roomId)
    if (isNaN(roomId)) {
      return res.status(400).json({ success: false, error: 'Invalid roomId' })
    }

    const db = require('../services/database')
    const pool = await db.getMysqlPool()

    const [danmakuCount] = await pool.execute(
      'SELECT COUNT(*) AS cnt FROM danmaku WHERE room_id = ?', [roomId]
    )
    const [giftStats] = await pool.execute(
      'SELECT COUNT(*) AS cnt, COALESCE(SUM(total_price), 0) AS total FROM gift_record WHERE room_id = ?', [roomId]
    )
    const [roomInfo] = await pool.execute(
      'SELECT online_count, peak_online FROM rooms WHERE id = ?', [roomId]
    )
    const [streamInfo] = await pool.execute(
      'SELECT COALESCE(MAX(duration_sec), 0) AS duration_sec FROM stream_records WHERE room_id = ? AND duration_sec IS NOT NULL', [roomId]
    )

    res.json({
      success: true,
      data: {
        room_id: roomId,
        total_danmaku: danmakuCount[0]?.cnt || 0,
        total_gifts: giftStats[0]?.cnt || 0,
        total_gift_value: (giftStats[0]?.total || 0).toFixed(2),
        online_count: roomInfo[0]?.online_count || 0,
        peak_online: roomInfo[0]?.peak_online || 0,
        stream_duration_sec: streamInfo[0]?.duration_sec || 0
      }
    })
  } catch (error) {
    console.error('Get room stats error:', error)
    res.status(500).json({ success: false, error: error.message })
  }
})

module.exports = router
