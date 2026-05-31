<script setup lang="ts">
import { ref } from 'vue'
import { useGiftStore } from '@/stores/gift'
import GiftItem from './GiftItem.vue'

const props = withDefaults(defineProps<{
  show: boolean
  roomId: string
  canSend?: boolean
}>(), {
  canSend: true
})

const emit = defineEmits<{
  close: []
}>()

const giftStore = useGiftStore()

const countOptions = [1, 5, 10, 66]
const sendError = ref('')

function handleSend() {
  const result = giftStore.sendGift(props.roomId)
  if (!result.success) {
    sendError.value = result.error
    setTimeout(() => { sendError.value = '' }, 3000)
  } else {
    sendError.value = ''
  }
}
</script>

<template>
  <div v-if="show" class="flex flex-col h-full">
    <div class="flex items-center justify-between px-4 py-3 border-b border-gray-800/50 bg-black/30">
      <div class="flex items-center gap-2">
        <span class="text-xl">🎁</span>
        <span class="font-semibold">Send Gift</span>
      </div>
      <button
        @click="emit('close')"
        class="p-1 hover:bg-gray-700/50 rounded transition-colors"
      >
        <span class="text-lg">✕</span>
      </button>
    </div>

    <div class="flex-1 overflow-y-auto p-4">
      <div class="grid grid-cols-3 gap-3">
        <GiftItem
          v-for="gift in giftStore.giftList"
          :key="gift.id"
          :gift="gift"
          :selected="giftStore.selectedGift?.id === gift.id"
          :count="giftStore.sendCount"
          @click="giftStore.selectGift(gift)"
        />
      </div>
    </div>

    <div class="p-4 border-t border-gray-800/50 bg-black/30 space-y-3">
      <div class="flex gap-2">
        <button
          v-for="n in countOptions"
          :key="n"
          @click="giftStore.setCount(n)"
          :class="[
            'flex-1 py-1.5 rounded-lg text-sm font-medium transition-all',
            giftStore.sendCount === n
              ? 'bg-pink-500/30 text-pink-300 ring-1 ring-pink-400'
              : 'bg-gray-800/50 text-gray-400 hover:bg-gray-700/50'
          ]"
        >
          ×{{ n }}
        </button>
      </div>

      <div v-if="giftStore.selectedGift" class="flex items-center justify-between text-sm">
        <span class="text-gray-400">
          {{ giftStore.selectedGift.giftIcon }} {{ giftStore.selectedGift.giftName }} ×{{ giftStore.sendCount }}
        </span>
        <span class="text-cyan-400 font-semibold">${{ giftStore.totalPrice.toFixed(2) }}</span>
      </div>

      <div v-if="sendError" class="text-center text-xs text-red-400 bg-red-500/10 rounded py-1.5 animate-pulse">
        {{ sendError }}
      </div>

      <button
        v-if="canSend"
        @click="handleSend"
        :disabled="!giftStore.selectedGift"
        class="w-full py-2.5 rounded-lg font-semibold text-sm transition-all bg-gradient-to-r from-pink-500 to-purple-500 hover:shadow-lg hover:shadow-pink-500/30 active:scale-95 disabled:opacity-40 disabled:cursor-not-allowed"
      >
        Send Gift
      </button>
      <div v-else class="w-full py-2.5 rounded-lg text-center text-sm text-gray-500 bg-gray-800/30">
        主播不能给自己送礼物
      </div>
    </div>
  </div>
</template>
