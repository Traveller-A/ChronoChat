<template>
  <StarField />
  <div id="app-container">
    <router-view v-slot="{ Component }">
      <transition name="route-fade" mode="out-in">
        <component :is="Component" />
      </transition>
    </router-view>
  </div>
</template>

<script setup>
import StarField from '@/components/StarField.vue'
// ChronoChat · 星轨观测台 · 根组件
</script>

<style scoped>
#app-container {
  position: relative;
  z-index: 1;
  width: 100%;
  min-height: 100vh;
}
</style>
