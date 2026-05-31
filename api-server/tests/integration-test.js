const http = require('http')

const API_BASE = process.env.API_BASE || 'http://localhost:3000'
let passed = 0
let failed = 0
const results = []

async function test(name, fn) {
  try {
    await fn()
    passed++
    results.push({ name, status: 'PASS' })
    console.log(`  ✅ ${name}`)
  } catch (e) {
    failed++
    results.push({ name, status: 'FAIL', error: e.message })
    console.log(`  ❌ ${name}: ${e.message}`)
  }
}

function get(path) {
  return new Promise((resolve, reject) => {
    http.get(`${API_BASE}${path}`, (res) => {
      let data = ''
      res.on('data', c => data += c)
      res.on('end', () => {
        try { resolve(JSON.parse(data)) }
        catch (e) { resolve(data) }
      })
    }).on('error', reject)
  })
}

function post(path, body) {
  return request('POST', path, body)
}

function put(path, body) {
  return request('PUT', path, body)
}

function request(method, path, body) {
  return new Promise((resolve, reject) => {
    const payload = body ? JSON.stringify(body) : null
    const options = {
      method,
      headers: { 'Content-Type': 'application/json' }
    }
    if (payload) options.headers['Content-Length'] = Buffer.byteLength(payload)
    
    const req = http.request(`${API_BASE}${path}`, options, (res) => {
      let data = ''
      res.on('data', c => data += c)
      res.on('end', () => {
        try { resolve(JSON.parse(data)) }
        catch (e) { resolve(data) }
      })
    })
    req.on('error', reject)
    if (payload) req.write(payload)
    req.end()
  })
}

// --- Wait for server to start ---
async function waitForServer(maxRetries = 15) {
  for (let i = 0; i < maxRetries; i++) {
    try {
      await new Promise((resolve, reject) => {
        const req = http.get(`${API_BASE}/api/health`, (res) => {
          let d = ''
          res.on('data', c => d += c)
          res.on('end', () => resolve(JSON.parse(d)))
        })
        req.on('error', reject)
        req.setTimeout(2000, () => { req.destroy(); reject(new Error('timeout')) })
      })
      return true
    } catch (e) {
      await new Promise(r => setTimeout(r, 1000))
    }
  }
  return false
}

async function main() {
  console.log('🚀 AI Live Room - Integration Test Suite\n')
  console.log('Waiting for API server on port 3000...')
  
  const serverReady = await waitForServer()
  if (!serverReady) {
    console.log('❌ Server did not start. Please start the server manually:')
    console.log('   cd api-server && node src/server.js')
    return
  }
  
  console.log('✅ Server is running!\n')
  console.log('--- Module 01: Backend Gift System ---')

  await test('T1.2 - HTTP GET /api/gifts returns gift list', async () => {
    const res = await get('/api/gifts')
    if (!res.success) throw new Error('success=false')
    if (!Array.isArray(res.data)) throw new Error('data is not an array')
    if (res.data.length < 6) throw new Error(`Expected >=6 gifts, got ${res.data.length}`)
    const rocket = res.data.find(g => g.giftName === '火箭')
    if (!rocket) throw new Error('Rocket gift not found')
    if (rocket.price !== 100) throw new Error(`Expected price 100, got ${rocket.price}`)
  })

  await test('T1.3 - HTTP POST /api/rooms/1/gift sends gift', async () => {
    const res = await post('/api/rooms/1/gift', {
      giftId: 1, count: 3, userId: 2, username: 'TestUser'
    })
    if (!res.success) throw new Error(`success=false: ${res.error}`)
    if (!res.data.id) throw new Error('No data.id returned')
    if (res.data.totalPrice !== 3) throw new Error(`Expected totalPrice=3, got ${res.data.totalPrice}`)
    if (res.data.effectType !== 'normal') throw new Error(`Expected effectType=normal`)
  })

  await test('T1.4 - HTTP GET /api/rooms/1/gift/rank returns rankings (graceful Redis fallback)', async () => {
    const res = await get('/api/rooms/1/gift/rank')
    if (!res.success) throw new Error('success=false')
    if (!Array.isArray(res.data)) throw new Error('data is not an array')
  })

  await test('T1.5a - Missing giftId returns error', async () => {
    const res = await post('/api/rooms/1/gift', { count: 1, userId: 2, username: 'Test' })
    if (res.success) throw new Error('Should have failed')
  })

  await test('T1.5b - Invalid giftId returns error', async () => {
    const res = await post('/api/rooms/1/gift', {
      giftId: 999, count: 1, userId: 2, username: 'Test'
    })
    if (res.success) throw new Error('Should have failed')
  })

  console.log('\n--- Module 03: Backend Room CRUD ---')

  await test('T3.2 - GET /api/rooms returns room list', async () => {
    const res = await get('/api/rooms')
    if (!res.success) throw new Error('success=false')
    if (!res.data.rooms) throw new Error('No rooms array')
    if (!Array.isArray(res.data.rooms)) throw new Error('rooms is not array')
    const aiRoom = res.data.rooms.find(r => r.roomName === 'AI直播间')
    if (!aiRoom) throw new Error('AI直播間 room not found')
  })

  await test('T3.3 - POST /api/rooms creates room', async () => {
    const res = await post('/api/rooms', {
      roomName: 'IntegrationTestRoom',
      hostId: 1,
      hostName: 'TestHost',
      title: 'Integration Test Room',
      coverImage: 'https://via.placeholder.com/400x225'
    })
    if (!res.success) throw new Error(`success=false: ${res.error}`)
    if (res.data.state !== 'living') throw new Error(`Expected state=living, got ${res.data.state}`)
    if (res.data.roomName !== 'IntegrationTestRoom') throw new Error('Wrong roomName')
  })

  await test('T3.4a - Close room with wrong hostId fails', async () => {
    const listRes = await get('/api/rooms')
    const testRoom = listRes.data.rooms.find(r => r.roomName === 'IntegrationTestRoom')
    if (!testRoom) throw new Error('Test room not found')
    
    const closeRes = await put(`/api/rooms/${testRoom.roomId}/close`, { hostId: 999 })
    if (closeRes.success) throw new Error('Should have failed with wrong hostId')
  })

  await test('T3.4b - Close room with correct hostId succeeds', async () => {
    const listRes = await get('/api/rooms')
    const testRoom = listRes.data.rooms.find(r => r.roomName === 'IntegrationTestRoom')
    if (!testRoom) throw new Error('Test room not found')
    
    const closeRes = await put(`/api/rooms/${testRoom.roomId}/close`, { hostId: 1 })
    if (!closeRes.success) throw new Error('Close room failed')
    
    const singleRes = await get(`/api/rooms/${testRoom.roomId}`)
    if (singleRes.data && singleRes.data.state !== 'closed') throw new Error('Room state not updated to closed')
  })

  await test('T3.6 - GET /api/rooms/:roomId/members returns members', async () => {
    const res = await get('/api/rooms/1/members')
    if (!res.success) throw new Error('success=false')
    if (!Array.isArray(res.data)) throw new Error('data is not array')
  })

  console.log('\n--- Existing API (Regression) ---')

  await test('R1 - GET /api/health returns ok', async () => {
    const res = await get('/api/health')
    if (res.status !== 'ok') throw new Error(`Expected status=ok, got ${res.status}`)
    if (!res.version) throw new Error('No version field')
  })

  await test('R2 - POST /api/auth/login creates/returns user', async () => {
    const res = await post('/api/auth/login', { username: 'TestUser123' })
    if (!res.success) throw new Error(`Login failed: ${res.error}`)
    if (!res.data.token) throw new Error('No token returned')
  })

  await test('R3 - GET /api/rooms/1 returns room info', async () => {
    const res = await get('/api/rooms/1')
    if (!res.success) throw new Error('success=false')
    if (!res.data.roomName) throw new Error('No roomName')
  })

  await test('R4 - GET /api/rooms/1/danmaku returns messages', async () => {
    const res = await get('/api/rooms/1/danmaku?limit=10')
    if (!res.success) throw new Error('success=false')
    if (!Array.isArray(res.data)) throw new Error('data is not array')
  })

  await test('R5 - POST /api/rooms/1/danmaku saves message', async () => {
    const res = await post('/api/rooms/1/danmaku', {
      content: 'Hello from test!',
      userId: 1,
      username: 'TestBot',
      color: '#00ff41'
    })
    if (!res.success) throw new Error(`Failed: ${res.error}`)
    if (res.data.content !== 'Hello from test!') throw new Error('Content mismatch')
  })

  // Summary
  console.log(`\n${'='.repeat(50)}`)
  console.log(`RESULTS: ${passed} passed, ${failed} failed, ${passed + failed} total`)
  
  if (failed > 0) {
    console.log('\n❌ FAILED TESTS:')
    results.filter(r => r.status === 'FAIL').forEach(r => {
      console.log(`  - ${r.name}: ${r.error}`)
    })
  } else {
    console.log('\n🎉 ALL TESTS PASSED!')
  }
  console.log('='.repeat(50))
}

main().catch(e => console.error('Test runner error:', e))
