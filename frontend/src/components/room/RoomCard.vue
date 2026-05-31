<script setup lang="ts">

defineProps<{
  roomId: number
  roomName: string
  hostName: string
  title?: string
  onlineCount: number
  coverImage?: string
  state: 'idle' | 'living' | 'closed'
}>()

function getAvatarUrl(name: string): string {
  return `https://api.dicebear.com/7.x/avataaars/svg?seed=${encodeURIComponent(name)}`
}
</script>

<template>
  <div
    class="bg-gray-900/80 backdrop-blur-sm border border-gray-800/50 rounded-xl overflow-hidden cursor-pointer hover:scale-105 hover:border-cyan-400/50 hover:shadow-lg hover:shadow-cyan-500/20 transition-all duration-300 group"
  >
    <div class="relative aspect-video bg-gradient-to-br from-gray-800 to-gray-900 overflow-hidden">
      <img
        v-if="coverImage"
        :src="coverImage"
        :alt="roomName"
        class="w-full h-full object-cover"
      />
      <div v-else class="w-full h-full flex items-center justify-center">
        <span class="text-4xl opacity-30">📡</span>
      </div>

      <div class="absolute top-3 left-3">
        <span
          v-if="state === 'living'"
          class="bg-red-600/90 text-white px-2 py-0.5 rounded-full text-xs animate-pulse"
        >
          ● LIVE
        </span>
        <span v-else class="bg-gray-600/80 text-gray-300 px-2 py-0.5 rounded-full text-xs">
          {{ state }}
        </span>
      </div>

      <div class="absolute bottom-3 right-3">
        <span class="bg-black/60 text-white px-2 py-0.5 rounded text-xs">
          👥 {{ onlineCount }}
        </span>
      </div>
    </div>

    <div class="p-4">
      <h3 class="font-semibold text-white truncate group-hover:text-cyan-400 transition-colors">
        {{ roomName }}
      </h3>
      <p v-if="title" class="text-xs text-gray-500 mt-0.5 truncate">{{ title }}</p>

      <div class="flex items-center gap-2 mt-3">
        <img
          :src="getAvatarUrl(hostName)"
          :alt="hostName"
          class="w-6 h-6 rounded-full bg-gray-700"
        />
        <span class="text-sm text-gray-400 truncate">{{ hostName }}</span>
      </div>
    </div>
  </div>
</template>
