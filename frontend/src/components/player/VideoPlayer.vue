<script setup lang="ts">
import { ref, computed, watch, onBeforeUnmount, nextTick } from 'vue'
import mpegts from 'mpegts.js'

const props = defineProps<{
  streamUrl?: string
  flvUrl?: string
}>()

const videoRef = ref<HTMLVideoElement | null>(null)
const videoError = ref(false)
const useFlv = ref(false)
const isMuted = ref(true)
const hasVideoTrack = ref(true)
const debugInfo = ref('')
let flvPlayer: mpegts.Player | null = null

function toggleMute() {
  isMuted.value = !isMuted.value
  if (videoRef.value) {
    videoRef.value.muted = isMuted.value
  }
}

const demoStream = 'https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8'

const activeStream = computed(() => {
  if (props.flvUrl) return props.flvUrl
  if (props.streamUrl) return props.streamUrl
  return demoStream
})

const isFlvSource = computed(() => {
  const url = activeStream.value
  return url.endsWith('.flv') && mpegts.isSupported()
})

function destroyFlvPlayer() {
  if (flvPlayer) {
    flvPlayer.pause()
    flvPlayer.detachMediaElement()
    flvPlayer.destroy()
    flvPlayer = null
  }
}

function startFlvPlayback(url: string) {
  destroyFlvPlayer()
  if (!videoRef.value) return

  useFlv.value = true
  videoError.value = false
  hasVideoTrack.value = true
  debugInfo.value = 'FLV connecting...'

  flvPlayer = mpegts.createPlayer({
    type: 'flv',
    url,
    isLive: true
  }, {
    enableWorker: true,
    lazyLoad: false,
    stashInitialSize: 128,
    seekType: 'range'
  })

  flvPlayer.on(mpegts.Events.ERROR, (errType, errDetail) => {
    console.error('[VideoPlayer] FLV error:', errType, errDetail)
    debugInfo.value = `FLV error: ${errType}`
    videoError.value = true
  })

  flvPlayer.on(mpegts.Events.MEDIA_INFO, (info) => {
    console.log('[VideoPlayer] FLV media info:', info)
    if (!info.hasVideo) {
      console.warn('[VideoPlayer] No video track in FLV stream')
      hasVideoTrack.value = false
      debugInfo.value = 'Audio only - no video track'
    } else {
      hasVideoTrack.value = true
      debugInfo.value = `Video: ${info.width}x${info.height} | ${info.videoCodec || 'unknown codec'}`
    }
    videoError.value = false
  })

  flvPlayer.on(mpegts.Events.STATISTICS_INFO, (stats) => {
    const s = stats as any
    if (s.videoBytesPerSecond === 0 && s.audioBytesPerSecond > 0) {
      hasVideoTrack.value = false
      debugInfo.value = 'No video data received'
    } else if (s.videoBytesPerSecond > 0) {
      hasVideoTrack.value = true
    }
  })

  flvPlayer.attachMediaElement(videoRef.value)
  flvPlayer.load()
  flvPlayer.play()
}

function stopFlvPlayback() {
  destroyFlvPlayer()
  useFlv.value = false
}

watch(() => [props.streamUrl, props.flvUrl], async ([newStreamUrl, newFlvUrl]) => {
  videoError.value = false
  const url = newFlvUrl || newStreamUrl || demoStream

  // #region debug-point C:play-url
  fetch("http://127.0.0.1:7778/event",{method:"POST",body:JSON.stringify({sessionId:"stream-viewer-no-video",runId:"pre",hypothesisId:"C",location:"VideoPlayer.vue:watch",msg:"[DEBUG] Play URL resolved",data:{flvUrl:newFlvUrl,hlsUrl:newStreamUrl,finalUrl:url,isFlv:url.endsWith('.flv'),flvSupported:mpegts.isSupported()},ts:Date.now()})}).catch(()=>{});
  // #endregion

  if (url.endsWith('.flv') && mpegts.isSupported()) {
    await nextTick()
    startFlvPlayback(url)
  } else {
    stopFlvPlayback()
    if (videoRef.value) {
      videoRef.value.load()
    }
  }
}, { immediate: true })

onBeforeUnmount(() => {
  destroyFlvPlayer()
})
</script>

<template>
  <div class="absolute inset-0 flex items-center justify-center bg-gradient-to-br from-gray-900 via-[#0F0F23] to-gray-900">
    <video
      v-show="!videoError"
      ref="videoRef"
      class="w-full h-full object-cover"
      playsinline
      autoplay
      muted
    />

    <div v-show="videoError" class="absolute inset-0 flex flex-col items-center justify-center gap-4 z-10 bg-[#0F0F23]">
      <div class="relative w-48 h-48">
        <div class="absolute inset-0 rounded-full border-2 border-cyan-400/30 animate-ping" />
        <div class="absolute inset-2 rounded-full border border-purple-500/20" />
        <div class="absolute inset-0 flex items-center justify-center">
          <div class="text-center space-y-2">
            <div class="w-24 h-24 mx-auto rounded-full bg-gradient-to-br from-cyan-400/10 to-purple-500/10 flex items-center justify-center">
              <div class="grid grid-cols-3 grid-rows-3 gap-0.5 opacity-40">
                <div v-for="i in 9" :key="i" class="w-4 h-4 rounded-sm bg-gradient-to-br from-cyan-400/80 to-purple-500/80" />
              </div>
            </div>
          </div>
        </div>
      </div>
      <p class="text-cyan-400/80 text-sm font-mono">STREAM CONNECTING...</p>
      <div class="flex items-center gap-1">
        <span class="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" style="animation-delay: 0ms" />
        <span class="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" style="animation-delay: 200ms" />
        <span class="w-2 h-2 rounded-full bg-cyan-400 animate-pulse" style="animation-delay: 400ms" />
      </div>
    </div>

    <div class="absolute top-4 left-4 flex items-center gap-2">
      <div class="flex items-center gap-1 px-2 py-1 bg-black/40 rounded">
        <span class="w-2 h-2 rounded-full bg-red-500 animate-pulse" />
        <span class="text-red-400 text-xs font-mono">REC</span>
      </div>
      <span class="text-gray-500 text-xs font-mono">00:00:00</span>
      <span v-if="useFlv" class="ml-1 px-1.5 py-0.5 bg-cyan-900/60 rounded text-cyan-300 text-[10px] font-mono">FLV</span>
      <span v-else class="ml-1 px-1.5 py-0.5 bg-purple-900/60 rounded text-purple-300 text-[10px] font-mono">HLS</span>
      <span v-if="!hasVideoTrack && useFlv && !videoError" class="ml-1 px-1.5 py-0.5 bg-yellow-900/60 rounded text-yellow-300 text-[10px] font-mono" :title="debugInfo">⚠ NO VIDEO</span>
    </div>

    <div class="absolute top-4 right-4 flex items-center gap-2">
      <button @click="toggleMute" class="px-2 py-1 bg-black/40 hover:bg-black/60 rounded text-xs font-mono transition-colors">
        {{ isMuted ? '🔇 取消静音' : '🔊 有声' }}
      </button>
      <span class="px-2 py-1 bg-black/40 rounded text-cyan-400 text-xs font-mono">1080p</span>
    </div>
  </div>
</template>
