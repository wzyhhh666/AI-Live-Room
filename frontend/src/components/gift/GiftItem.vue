<script setup lang="ts">
import type { GiftItem } from '@/stores/gift'

defineProps<{
  gift: GiftItem
  selected: boolean
  count: number
}>()
</script>

<template>
  <div
    class="flex flex-col items-center justify-center p-3 rounded-xl cursor-pointer transition-all duration-200 select-none"
    :class="selected
      ? 'ring-2 ring-pink-400 bg-pink-500/10 scale-105'
      : 'border border-gray-700/50 hover:border-cyan-400/50 hover:bg-gray-800/30'"
  >
    <span class="text-3xl mb-1">{{ gift.giftIcon }}</span>
    <span class="text-xs font-medium text-gray-300 truncate w-full text-center">{{ gift.giftName }}</span>
    <span class="text-xs text-cyan-400 mt-0.5">${{ gift.price.toFixed(2) }}</span>
  </div>
</template>
