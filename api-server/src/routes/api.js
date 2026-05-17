const express = require('express')
const router = express.Router()
const roomService = require('../services/room')
const userService = require('../services/user')

router.get('/health', (req, res) => {
  res.json({ 
    status: 'ok', 
    timestamp: new Date().toISOString(),
    version: '1.0.0'
  })
})

router.post('/auth/login', async (req, res) => {
  try {
    const { username, password } = req.body
    
    if (!username) {
      return res.status(400).json({
        success: false,
        error: 'Username is required'
      })
    }
    
    const result = await userService.login(username, password)
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

module.exports = router
