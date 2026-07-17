<template>
  <div class="starfield" aria-hidden="true">
    <!-- 稳定星场（缓慢漂移，可感知） -->
    <div class="layer drift-1"></div>
    <div class="layer drift-2"></div>
    <div class="layer drift-3"></div>
    <!-- 闪烁星层（不同节奏，叠加呈随机闪烁） -->
    <div class="layer twinkle t-a"></div>
    <div class="layer twinkle t-b"></div>
    <div class="layer twinkle t-c"></div>
    <!-- 星云 -->
    <div class="nebula"></div>
    <!-- 发光亮星（脉冲） -->
    <span class="bright-star s1"></span>
    <span class="bright-star s2"></span>
    <span class="bright-star s3"></span>
    <span class="bright-star s4"></span>
    <span class="bright-star s5"></span>
    <span class="bright-star s6"></span>
    <!-- 流星：左上 -> 右下，各自轨迹不同 -->
    <span class="shooting-star ss1"></span>
    <span class="shooting-star ss2"></span>
    <span class="shooting-star ss3"></span>
    <!-- 流星：顶部中间 -> 右下 -->
    <span class="shooting-star ss4"></span>
    <span class="shooting-star ss5"></span>
  </div>
</template>

<script setup>
// 星轨观测台 · 深空星点背景
// 缓慢漂移 + 闪烁 + 发光亮星 + 左上至右下流星；reduced-motion 下全部静态
</script>

<style scoped>
.starfield {
  position: fixed;
  inset: 0;
  z-index: 0;
  pointer-events: none;
  background: radial-gradient(ellipse at 22% 0%, #1c2945 0%, #141d33 48%, #0e1426 100%);
}

.layer {
  position: absolute;
  inset: 0;
  background-repeat: repeat;
  will-change: background-position;
}

/* ---- 稳定漂移层（密集小星，缓慢可感知） ---- */
.drift-1 {
  background-image:
    radial-gradient(1px 1px at 20px 30px, #eae6dd, transparent),
    radial-gradient(1px 1px at 80px 120px, #c8cdd9, transparent),
    radial-gradient(1px 1px at 150px 80px, #eae6dd, transparent),
    radial-gradient(1.5px 1.5px at 200px 50px, rgba(230, 181, 102, 0.9), transparent),
    radial-gradient(1px 1px at 60px 200px, #9aa3b8, transparent),
    radial-gradient(1px 1px at 230px 220px, #c8cdd9, transparent),
    radial-gradient(1px 1px at 120px 260px, #eae6dd, transparent);
  background-size: 250px 250px;
  opacity: 0.9;
  animation: drift1 60s linear infinite;
}

.drift-2 {
  background-image:
    radial-gradient(1px 1px at 40px 60px, rgba(94, 179, 196, 0.9), transparent),
    radial-gradient(1px 1px at 120px 180px, #eae6dd, transparent),
    radial-gradient(1.5px 1.5px at 220px 140px, rgba(230, 181, 102, 0.85), transparent),
    radial-gradient(1px 1px at 300px 40px, #c8cdd9, transparent),
    radial-gradient(1px 1px at 330px 300px, #9aa3b8, transparent),
    radial-gradient(1px 1px at 200px 320px, #eae6dd, transparent);
  background-size: 360px 360px;
  opacity: 0.55;
  animation: drift2 85s linear infinite;
}

.drift-3 {
  background-image:
    radial-gradient(2px 2px at 100px 100px, rgba(230, 181, 102, 0.7), transparent),
    radial-gradient(1.5px 1.5px at 400px 250px, rgba(94, 179, 196, 0.65), transparent),
    radial-gradient(2px 2px at 250px 400px, rgba(234, 230, 221, 0.75), transparent),
    radial-gradient(1.5px 1.5px at 460px 80px, rgba(234, 230, 221, 0.6), transparent);
  background-size: 520px 520px;
  opacity: 0.65;
  animation: drift3 125s linear infinite;
}

/* ---- 闪烁层（稀疏亮星，opacity 脉动） ---- */
.twinkle { background-size: 340px 340px; }
.t-a {
  background-image:
    radial-gradient(1.4px 1.4px at 70px 90px, #ffffff, transparent),
    radial-gradient(1.2px 1.2px at 240px 200px, rgba(230, 181, 102, 0.95), transparent),
    radial-gradient(1.4px 1.4px at 150px 300px, #ffffff, transparent);
  animation: twinkle 4s ease-in-out infinite;
}
.t-b {
  background-image:
    radial-gradient(1.3px 1.3px at 180px 60px, rgba(94, 179, 196, 0.95), transparent),
    radial-gradient(1.4px 1.4px at 300px 260px, #ffffff, transparent),
    radial-gradient(1.2px 1.2px at 60px 240px, rgba(230, 181, 102, 0.9), transparent);
  animation: twinkle 5.5s ease-in-out infinite 1s;
}
.t-c {
  background-image:
    radial-gradient(1.5px 1.5px at 120px 160px, #ffffff, transparent),
    radial-gradient(1.3px 1.3px at 280px 120px, rgba(94, 179, 196, 0.9), transparent);
  background-size: 300px 300px;
  animation: twinkle 7s ease-in-out infinite 2s;
}

@keyframes twinkle {
  0%, 100% { opacity: 0.15; }
  50% { opacity: 0.95; }
}

/* ---- 星云（极慢漂移） ---- */
.nebula {
  position: absolute;
  inset: -10%;
  background:
    radial-gradient(ellipse 55% 42% at 78% 12%, rgba(94, 179, 196, 0.11), transparent 60%),
    radial-gradient(ellipse 45% 38% at 12% 82%, rgba(230, 181, 102, 0.1), transparent 60%),
    radial-gradient(ellipse 40% 30% at 50% 50%, rgba(120, 110, 180, 0.05), transparent 70%);
  animation: nebula-drift 240s linear infinite;
}

/* ---- 发光亮星（脉冲） ---- */
.bright-star {
  position: absolute;
  width: 3px;
  height: 3px;
  border-radius: 50%;
  background: #ffffff;
  box-shadow: 0 0 6px 1px rgba(255, 255, 255, 0.9), 0 0 14px 3px rgba(230, 181, 102, 0.5);
  animation: star-pulse 5s ease-in-out infinite;
}
.s1 { top: 14%; left: 18%; animation-duration: 4.5s; }
.s2 { top: 22%; left: 82%; background: rgba(230, 181, 102, 0.95); box-shadow: 0 0 6px 1px rgba(230, 181, 102, 0.9), 0 0 16px 4px rgba(230, 181, 102, 0.5); animation-duration: 6s; animation-delay: 1.2s; }
.s3 { top: 62%; left: 8%; background: rgba(94, 179, 196, 0.95); box-shadow: 0 0 6px 1px rgba(94, 179, 196, 0.9), 0 0 16px 4px rgba(94, 179, 196, 0.5); animation-duration: 5.5s; animation-delay: 0.6s; }
.s4 { top: 78%; left: 72%; animation-duration: 7s; animation-delay: 2s; }
.s5 { top: 40%; left: 90%; background: rgba(230, 181, 102, 0.95); box-shadow: 0 0 6px 1px rgba(230, 181, 102, 0.9), 0 0 14px 3px rgba(230, 181, 102, 0.45); animation-duration: 4s; animation-delay: 1.8s; }
.s6 { top: 88%; left: 38%; background: rgba(94, 179, 196, 0.95); box-shadow: 0 0 6px 1px rgba(94, 179, 196, 0.9), 0 0 14px 3px rgba(94, 179, 196, 0.45); animation-duration: 6.5s; animation-delay: 0.3s; }

@keyframes star-pulse {
  0%, 100% { opacity: 0.35; transform: scale(0.85); }
  50% { opacity: 1; transform: scale(1.15); }
}

/* ---- 流星：左上 -> 右下 ---- */
.shooting-star {
  --ang: 28deg;
  --dist: 540px;
  position: absolute;
  width: 150px;
  height: 2px;
  border-radius: 999px;
  /* 尾部：从头部(右)的白向左渐隐 */
  background: linear-gradient(90deg, rgba(255, 255, 255, 0) 0%, rgba(255, 255, 255, 0.55) 55%, rgba(255, 255, 255, 0.95) 100%);
  opacity: 0;
  animation: shoot 16s ease-in infinite;
}
/* 流星头部：发光星点，与尾部相匹配 */
.shooting-star::after {
  content: '';
  position: absolute;
  right: -1px;
  top: 50%;
  width: 5px;
  height: 5px;
  border-radius: 50%;
  background: #ffffff;
  box-shadow: 0 0 7px 1px rgba(255, 255, 255, 0.95), 0 0 16px 4px rgba(230, 181, 102, 0.55);
  transform: translateY(-50%);
}
.ss1 { top: 5%;  left: 6%;  --ang: 28deg; --dist: 540px; animation-duration: 15s; animation-delay: 2s; }
.ss2 { top: 13%; left: 2%;  --ang: 34deg; --dist: 640px; animation-duration: 22s; animation-delay: 11s; }
.ss3 { top: 3%;  left: 20%; --ang: 23deg; --dist: 480px; animation-duration: 29s; animation-delay: 19s; }
.ss4 { top: 4%;  left: 44%; --ang: 26deg; --dist: 520px; animation-duration: 18s; animation-delay: 6s; }
.ss5 { top: 9%;  left: 52%; --ang: 32deg; --dist: 600px; animation-duration: 26s; animation-delay: 15s; }

@keyframes shoot {
  0%   { transform: rotate(var(--ang)) translateX(0); opacity: 0; }
  2%   { opacity: 1; }
  16%  { transform: rotate(var(--ang)) translateX(var(--dist)); opacity: 0; }
  100% { transform: rotate(var(--ang)) translateX(var(--dist)); opacity: 0; }
}

@keyframes drift1 { from { background-position: 0 0; } to { background-position: 250px 250px; } }
@keyframes drift2 { from { background-position: 0 0; } to { background-position: -360px 360px; } }
@keyframes drift3 { from { background-position: 0 0; } to { background-position: 520px -520px; } }
@keyframes nebula-drift { from { transform: translate(0, 0); } to { transform: translate(-40px, 30px); } }

@media (prefers-reduced-motion: reduce) {
  .layer, .bright-star, .shooting-star, .nebula { animation: none !important; }
  .twinkle { opacity: 0.6; }
  .bright-star { opacity: 0.7; }
  .shooting-star { opacity: 0; }
}
</style>
