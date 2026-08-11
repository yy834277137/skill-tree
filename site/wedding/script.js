(function () {
  'use strict';

  // ===== 配置 =====
  var CONFIG = {
    // 婚礼日期（修改为你的日期）
    weddingDate: '2026-10-01T10:00:00',
    // 照片数量（对应 photos/01.jpg ~ photos/04.jpg）
    photoCount: 4,
    // 轮播间隔（毫秒）
    slideInterval: 5000,
    // 腾讯收集表 URL（创建后在腾讯文档获取分享链接，替换此处）
    tencentFormUrl: '',
    // Cloudflare Worker URL（进阶方案，暂时留空）
    workerUrl: ''
  };

  // ===== 轮播 =====
  function initSlideshow() {
    var slides = document.querySelectorAll('.slide');
    var dots = document.querySelectorAll('.dot');
    if (slides.length < 2) return;

    var current = 0;
    var timer = null;

    function goTo(index) {
      slides[current].classList.remove('active');
      dots[current].classList.remove('active');
      current = index;
      slides[current].classList.add('active');
      dots[current].classList.add('active');
    }

    function next() {
      goTo((current + 1) % slides.length);
    }

    timer = setInterval(next, CONFIG.slideInterval);

    // 点击指示器切换
    dots.forEach(function (dot) {
      dot.addEventListener('click', function () {
        clearInterval(timer);
        goTo(parseInt(this.dataset.index));
        timer = setInterval(next, CONFIG.slideInterval);
      });
    });

    // 触摸滑动支持
    var touchStartX = 0;
    var touchEndX = 0;
    document.addEventListener('touchstart', function (e) {
      touchStartX = e.changedTouches[0].screenX;
    }, { passive: true });
    document.addEventListener('touchend', function (e) {
      touchEndX = e.changedTouches[0].screenX;
      var diff = touchStartX - touchEndX;
      if (Math.abs(diff) > 50) {
        clearInterval(timer);
        if (diff > 0) {
          goTo((current + 1) % slides.length);
        } else {
          goTo((current - 1 + slides.length) % slides.length);
        }
        timer = setInterval(next, CONFIG.slideInterval);
      }
    }, { passive: true });
  }

  // ===== 背景音乐 =====
  function initMusic() {
    var audio = document.getElementById('bgMusic');
    var btn = document.getElementById('musicBtn');
    if (!audio || !btn) return;

    // 首次用户交互后尝试自动播放
    var autoPlayAttempted = false;
    function tryAutoPlay() {
      if (autoPlayAttempted) return;
      autoPlayAttempted = true;
      audio.play().then(function () {
        btn.classList.remove('paused');
        btn.classList.add('playing');
      }).catch(function () {
        // 浏览器阻止自动播放，用户需手动点击
      });
    }

    document.addEventListener('click', tryAutoPlay, { once: true });
    document.addEventListener('touchstart', tryAutoPlay, { once: true });

    btn.addEventListener('click', function (e) {
      e.stopPropagation();
      if (audio.paused) {
        audio.play().then(function () {
          btn.classList.remove('paused');
          btn.classList.add('playing');
        }).catch(function () {});
      } else {
        audio.pause();
        btn.classList.remove('playing');
        btn.classList.add('paused');
      }
    });

    // 音量淡入
    audio.volume = 0;
    audio.addEventListener('play', function () {
      var vol = 0;
      var fadeIn = setInterval(function () {
        vol += 0.05;
        if (vol >= 0.5) {
          vol = 0.5;
          clearInterval(fadeIn);
        }
        audio.volume = vol;
      }, 100);
    });
  }

  // ===== 倒计时 =====
  function initCountdown() {
    var daysEl = document.getElementById('cd-days');
    var hoursEl = document.getElementById('cd-hours');
    var minsEl = document.getElementById('cd-mins');
    var secsEl = document.getElementById('cd-secs');
    if (!daysEl) return;

    var target = new Date(CONFIG.weddingDate).getTime();

    function tick() {
      var now = Date.now();
      var diff = target - now;

      if (diff <= 0) {
        daysEl.textContent = '00';
        hoursEl.textContent = '00';
        minsEl.textContent = '00';
        secsEl.textContent = '00';
        return;
      }

      var days = Math.floor(diff / (1000 * 60 * 60 * 24));
      var hours = Math.floor((diff % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60));
      var mins = Math.floor((diff % (1000 * 60 * 60)) / (1000 * 60));
      var secs = Math.floor((diff % (1000 * 60)) / 1000);

      daysEl.textContent = String(days).padStart(2, '0');
      hoursEl.textContent = String(hours).padStart(2, '0');
      minsEl.textContent = String(mins).padStart(2, '0');
      secsEl.textContent = String(secs).padStart(2, '0');
    }

    tick();
    setInterval(tick, 1000);
  }

  // ===== 花瓣动画 =====
  function initPetals() {
    var container = document.getElementById('petals');
    if (!container) return;

    var count = 15;
    for (var i = 0; i < count; i++) {
      var petal = document.createElement('div');
      petal.classList.add('petal');
      petal.style.left = Math.random() * 100 + '%';
      petal.style.animationDuration = (Math.random() * 8 + 10) + 's';
      petal.style.animationDelay = (Math.random() * 12) + 's';
      petal.style.fontSize = (Math.random() * 10 + 14) + 'px';
      container.appendChild(petal);
    }
  }

  // ===== 表单提交 =====
  function initForm() {
    var form = document.getElementById('rsvpForm');
    var btn = document.getElementById('submitBtn');
    var hint = document.getElementById('formHint');
    if (!form) return;

    form.addEventListener('submit', function (e) {
      e.preventDefault();

      var name = document.getElementById('name').value.trim();
      if (!name) {
        hint.textContent = '请填写您的姓名';
        hint.className = 'form-hint error';
        return;
      }

      btn.classList.add('loading');
      hint.textContent = '';
      hint.className = 'form-hint';

      var data = {
        name: name,
        phone: document.getElementById('phone').value.trim(),
        guests: document.getElementById('guests').value,
        relation: document.getElementById('relation').value,
        message: document.getElementById('message').value.trim()
      };

      // 策略1: 提交到 Cloudflare Worker
      if (CONFIG.workerUrl) {
        fetch(CONFIG.workerUrl, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(data)
        }).then(function (res) {
          btn.classList.remove('loading');
          if (res.ok) {
            hint.textContent = '登记成功！期待您的到来 ❤';
            hint.className = 'form-hint success';
            form.reset();
          } else {
            throw new Error('Server error');
          }
        }).catch(function () {
          btn.classList.remove('loading');
          hint.textContent = '提交失败，请稍后重试';
          hint.className = 'form-hint error';
        });
        return;
      }

      // 策略2: 跳转腾讯收集表（默认）
      if (CONFIG.tencentFormUrl) {
        window.open(CONFIG.tencentFormUrl, '_blank');
        hint.textContent = '已打开登记链接，请在新页面中填写';
        hint.className = 'form-hint success';
        btn.classList.remove('loading');
        return;
      }

      // 无配置：本地模拟
      setTimeout(function () {
        btn.classList.remove('loading');
        hint.textContent = '登记成功！期待您的到来 ❤';
        hint.className = 'form-hint success';
        form.reset();
      }, 600);
    });
  }

  // ===== 启动 =====
  initSlideshow();
  initMusic();
  initCountdown();
  initPetals();
  initForm();
})();
