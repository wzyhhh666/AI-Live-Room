<script setup lang="ts">
import { ref, watch, onMounted, onUnmounted } from 'vue'

const props = defineProps<{
  danmakuList: Array<{
    id: number
    username: string
    content: string
    color: string
    type?: string
  }>
}>()

const canvasRef = ref<HTMLCanvasElement | null>(null)
let animationId = 0
let ctx: CanvasRenderingContext2D | null = null
let canvasWidth = 0
let canvasHeight = 0

interface DanmakuItem {
  id: number
  text: string
  x: number
  y: number
  color: string
  speed: number
  width: number
  isEmoji: boolean
  bouncePhase: number
  fontSize: number
}

const activeItems: DanmakuItem[] = []
const TRACK_COUNT = 8
const TRACK_HEIGHT = 32
const usedTracks = new Set<number>()
let lastTrackIndex = 0

function getNextTrack(): number {
  usedTracks.clear()
  for (const item of activeItems) {
    usedTracks.add(Math.floor(item.y / TRACK_HEIGHT))
  }
  for (let i = 0; i < TRACK_COUNT; i++) {
    const candidate = (lastTrackIndex + i) % TRACK_COUNT
    if (!usedTracks.has(candidate)) {
      lastTrackIndex = candidate
      return candidate
    }
  }
  lastTrackIndex = (lastTrackIndex + 1) % TRACK_COUNT
  return lastTrackIndex
}

function addDanmaku(msg: { id: number; username: string; content: string; color: string; type?: string }) {
  if (!ctx) return
  const isEmoji = msg.type === 'emoji'
  const fontSize = isEmoji ? 40 : 16
  ctx.font = `500 ${fontSize}px "Microsoft YaHei", "Segoe UI Emoji", sans-serif`
  const text = isEmoji ? msg.content : `${msg.username}: ${msg.content}`
  const metrics = ctx.measureText(text)
  const track = getNextTrack()

  activeItems.push({
    id: msg.id,
    text,
    x: canvasWidth,
    y: track * TRACK_HEIGHT + (isEmoji ? 36 : 26),
    color: msg.color,
    speed: isEmoji ? 1.0 : 1.5 + Math.random() * 0.5,
    width: metrics.width,
    isEmoji,
    bouncePhase: isEmoji ? Math.random() * Math.PI * 2 : 0,
    fontSize
  })

  while (activeItems.length > 50) {
    activeItems.shift()
  }
}

function render() {
  if (!ctx || !canvasWidth || !canvasHeight) return

  ctx.clearRect(0, 0, canvasWidth, canvasHeight)

  for (let i = activeItems.length - 1; i >= 0; i--) {
    const item = activeItems[i]
    item.x -= item.speed

    if (item.x + item.width < 0) {
      activeItems.splice(i, 1)
      continue
    }

    ctx.save()
    ctx.font = `500 ${item.fontSize}px "Microsoft YaHei", "Segoe UI Emoji", sans-serif`
    ctx.shadowBlur = item.isEmoji ? 8 : 4
    ctx.shadowColor = item.isEmoji ? '#ffdd00' : item.color
    ctx.fillStyle = item.color

    if (item.isEmoji) {
      item.bouncePhase += 0.05
      const bounceY = Math.sin(item.bouncePhase) * 4
      ctx.fillText(item.text, item.x, item.y + bounceY)
    } else {
      ctx.fillText(item.text, item.x, item.y)
    }
    ctx.restore()
  }

  animationId = requestAnimationFrame(render)
}

function handleResize() {
  if (!canvasRef.value) return
  const parent = canvasRef.value.parentElement
  if (!parent) return
  canvasWidth = parent.clientWidth
  canvasHeight = parent.clientHeight
  canvasRef.value.width = canvasWidth
  canvasRef.value.height = canvasHeight
}

watch(
  () => props.danmakuList,
  (newList, oldList) => {
    if (!oldList) return
    if (!newList || newList.length === 0) return
    const newest = newList[newList.length - 1]
    if (newest && (!oldList.length || newest.id !== oldList[oldList.length - 1]?.id)) {
      addDanmaku(newest)
    }
  },
  { deep: false }
)

onMounted(() => {
  if (!canvasRef.value) return
  ctx = canvasRef.value.getContext('2d')
  handleResize()
  window.addEventListener('resize', handleResize)
  animationId = requestAnimationFrame(render)
})

onUnmounted(() => {
  cancelAnimationFrame(animationId)
  window.removeEventListener('resize', handleResize)
  activeItems.length = 0
})
</script>

<template>
  <canvas
    ref="canvasRef"
    class="absolute inset-0 pointer-events-none z-20"
  />
</template>
