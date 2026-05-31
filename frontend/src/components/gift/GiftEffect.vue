<script setup lang="ts">
import { watch, ref } from 'vue'
import { useGiftStore } from '@/stores/gift'

const giftStore = useGiftStore()
const effectText = ref('')
const effectStyle = ref<Record<string, string>>({})
const iconElements = ref<string[]>([])

watch(() => giftStore.lastGift, (event) => {
  if (!event) return

  effectText.value = `${event.giftIcon} ${event.senderName} sent ${event.giftName} ×${event.count}`
  effectStyle.value = {
    animation: `${event.effectType}-gift-effect 3s ease-out forwards`
  }

  if (event.effectType === 'rain' || event.effectType === 'explosion') {
    iconElements.value = Array.from({ length: 20 }, (_, i) =>
      `${event.giftIcon} ${i * 80}ms`
    )
  } else {
    iconElements.value = []
  }

  setTimeout(() => {
    effectText.value = ''
    iconElements.value = []
  }, 3500)
})
</script>

<template>
  <div class="absolute inset-0 pointer-events-none z-30 overflow-hidden">
    <div
      v-if="effectText"
      :style="effectStyle"
      class="absolute bottom-20 left-1/2 -translate-x-1/2 text-2xl font-bold text-white"
      style="text-shadow: 0 0 20px #ff006e, 0 0 40px #ff006e"
    >
      {{ effectText }}
    </div>

    <span
      v-for="(item, idx) in iconElements"
      :key="idx"
      class="absolute text-2xl"
      :style="{
        left: `${10 + Math.random() * 80}%`,
        top: '-10%',
        animation: `rain-fall 3s ease-in forwards`,
        animationDelay: `${idx * 80}ms`
      }"
    >
      {{ item.split(' ')[0] }}
    </span>
  </div>
</template>

<style scoped>
@keyframes normal-gift-effect {
  0% { opacity: 0; transform: translateX(100px) translateY(80px) scale(0.5); }
  20% { opacity: 1; transform: translateX(0) translateY(0) scale(1); }
  80% { opacity: 1; transform: translateX(0) translateY(0) scale(1); }
  100% { opacity: 0; transform: translateY(-60px) scale(1.2); }
}

@keyframes explosion-gift-effect {
  0% { opacity: 0; transform: scale(0) rotate(-10deg); }
  30% { opacity: 1; transform: scale(1.3) rotate(3deg); }
  60% { transform: scale(1) rotate(0deg); }
  100% { opacity: 0; transform: scale(1.5); }
}

@keyframes rocket-gift-effect {
  0% { opacity: 0; transform: translateY(100px) translateX(0); }
  30% { opacity: 1; transform: translateY(0) translateX(0); }
  80% { opacity: 1; }
  100% { opacity: 0; transform: translateY(-200px) translateX(0); }
}

@keyframes rain-fall {
  0% { opacity: 1; transform: translateY(-100%); }
  80% { opacity: 1; }
  100% { opacity: 0; transform: translateY(100vh); }
}
</style>
