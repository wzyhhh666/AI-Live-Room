<script setup lang="ts">
import { useGiftStore } from '@/stores/gift'
import { watch } from 'vue'

const giftStore = useGiftStore()

watch(() => giftStore.giftRank, (newVal) => {
  // #region debug-point D:giftrank-watch
  fetch("http://127.0.0.1:7778/event",{method:"POST",body:JSON.stringify({sessionId:"gift-rank-empty",runId:"pre",hypothesisId:"D",location:"GiftRank.vue:9",msg:"[DEBUG] GiftRank.vue giftRank changed",data:{length:newVal.length,data:newVal}})}).catch(()=>{});
  // #endregion
}, { deep: true, immediate: true })

function getRankEmoji(index: number): string {
  if (index === 0) return '🥇'
  if (index === 1) return '🥈'
  if (index === 2) return '🥉'
  return `${index + 1}`
}

function getRankClass(index: number): string {
  if (index === 0) return 'text-yellow-400'
  if (index === 1) return 'text-gray-300'
  if (index === 2) return 'text-amber-600'
  return 'text-gray-500'
}
</script>

<template>
  <div class="flex flex-col h-full">
    <div class="flex items-center gap-2 px-4 py-3 border-b border-gray-800/50 bg-black/30">
      <span class="text-lg">🏆</span>
      <span class="font-semibold">Gift Rankings</span>
    </div>

    <div class="flex-1 overflow-y-auto p-4 space-y-2">
      <div v-if="giftStore.giftRank.length === 0" class="text-center text-gray-500 mt-8">
        <p class="text-4xl mb-2">🎁</p>
        <p class="text-sm">No gifts yet. Be the first!</p>
      </div>

      <div
        v-for="(entry, index) in giftStore.giftRank"
        :key="entry.senderId"
        class="flex items-center gap-3 p-2 rounded-lg hover:bg-gray-800/30 transition-colors"
      >
        <span class="w-8 text-center font-bold" :class="getRankClass(index)">
          {{ getRankEmoji(index) }}
        </span>
        <div class="flex-1 min-w-0">
          <p class="text-sm font-medium truncate" :class="index < 3 ? getRankClass(index) : 'text-gray-300'">
            {{ entry.senderName }}
          </p>
        </div>
        <span class="text-sm text-cyan-400 font-semibold">${{ entry.totalAmount.toFixed(2) }}</span>
      </div>
    </div>
  </div>
</template>
