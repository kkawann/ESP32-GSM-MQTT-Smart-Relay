#include "html_page.h"

const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fa" dir="rtl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>Smart Relay Pro</title>
<style>
/* بقیه کدهای CSS سر جای خودشون می‌مونن */
:root{
--primary:#6c63ff;--primary-dark:#5a52d5;
--success:#00c851;--danger:#ff4444;
--warning:#ffbb33;--info:#33b5e5;
--bg:#0f0f1a;--card:#1a1a2e;--card2:#16213e;
--text:#e0e0e0;--text2:#a0a0b0;
--border:#2a2a4a;--radius:14px;
--shadow:0 8px 32px rgba(0,0,0,.4);
}
*{margin:0;padding:0;box-sizing:border-box;
  font-family:'Segoe UI',Tahoma,sans-serif;
  -webkit-tap-highlight-color:transparent}
body{background:var(--bg);color:var(--text);
     min-height:100vh;padding-bottom:80px}
::-webkit-scrollbar{width:4px}
::-webkit-scrollbar-track{background:var(--card)}
::-webkit-scrollbar-thumb{background:var(--primary);border-radius:4px}

/* ── Header ── */
.header{
  background:linear-gradient(135deg,var(--primary),#a855f7);
  padding:16px 20px 60px;position:relative;overflow:hidden;
}
.header::before{
  content:'';position:absolute;top:-50%;left:-50%;
  width:200%;height:200%;
  background:radial-gradient(circle,rgba(255,255,255,.05) 0%,transparent 70%);
}
.header h1{font-size:20px;font-weight:700;color:#fff;position:relative}
.header p{font-size:11px;color:rgba(255,255,255,.7);margin-top:3px;position:relative}
.header-badges{display:flex;gap:6px;margin-top:8px;position:relative;flex-wrap:wrap}
.hbadge{padding:3px 10px;border-radius:20px;font-size:10px;font-weight:700;
        background:rgba(255,255,255,.15);color:#fff;
        display:flex;align-items:center;gap:4px;}
.hbadge.ok{background:rgba(0,200,81,.3)}
.hbadge.err{background:rgba(255,68,68,.3)}

/* ── Bottom Nav ── */
.bottom-nav{
  position:fixed;bottom:0;left:0;right:0;
  background:var(--card);border-top:1px solid var(--border);
  display:flex;z-index:100;
  padding-bottom:env(safe-area-inset-bottom);
  box-shadow:0 -4px 20px rgba(0,0,0,.3);
}
.nav-btn{
  flex:1;padding:10px 4px 8px;border:none;background:none;
  color:var(--text2);cursor:pointer;transition:.3s;
  display:flex;flex-direction:column;align-items:center;gap:3px;
  font-size:9px;font-weight:600;
}
.nav-btn .icon{font-size:20px;transition:.3s}
.nav-btn.active{color:var(--primary)}
.nav-btn.active .icon{transform:translateY(-2px)}
.nav-indicator{
  position:absolute;top:0;height:2px;
  background:var(--primary);border-radius:0 0 2px 2px;transition:.3s;
}

/* ── Pages ── */
.page{display:none;padding:16px;margin-top:-44px}
.page.active{display:block}

/* ── Cards ── */
.card{background:var(--card);border-radius:var(--radius);
      padding:16px;margin-bottom:12px;
      border:1px solid var(--border);box-shadow:var(--shadow);}
.card-title{font-size:13px;font-weight:700;color:var(--text);
            margin-bottom:12px;display:flex;align-items:center;gap:8px;}
.card-title .icon{width:28px;height:28px;
  background:linear-gradient(135deg,var(--primary),#a855f7);
  border-radius:8px;display:flex;align-items:center;
  justify-content:center;font-size:14px;}

/* ── Relay Grid ── */
.relay-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.relay-card{background:var(--card2);border-radius:12px;padding:14px;
            border:1px solid var(--border);transition:.3s;
            cursor:pointer;position:relative;overflow:hidden;}
.relay-card.on{border-color:var(--success);
  box-shadow:0 0 20px rgba(0,200,81,.2);}
.relay-card.on::before{content:'';position:absolute;top:0;left:0;right:0;
  height:2px;background:linear-gradient(90deg,var(--success),#00ff88);}
.relay-name{font-size:12px;font-weight:700;margin-bottom:4px}
.relay-status{display:flex;align-items:center;gap:6px;
  font-size:10px;color:var(--text2);margin-bottom:10px;}
.dot{width:6px;height:6px;border-radius:50%;background:var(--danger)}
.dot.on{background:var(--success);box-shadow:0 0 6px var(--success);
  animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.5}}
.relay-toggle{width:100%;padding:8px;border:none;border-radius:8px;
  font-size:11px;font-weight:700;cursor:pointer;transition:.3s;}
.relay-toggle.off-btn{background:rgba(0,200,81,.15);color:var(--success)}
.relay-toggle.on-btn{background:rgba(255,68,68,.15);color:var(--danger)}
.relay-timer{font-size:9px;color:var(--warning);margin-top:6px;text-align:center;}

/* ── Quick Buttons ── */
.quick-row{display:flex;gap:8px;margin-bottom:12px;flex-wrap:wrap}
.quick-btn{flex:1;min-width:80px;padding:10px 8px;border:none;
  border-radius:10px;font-size:11px;font-weight:700;cursor:pointer;
  transition:.3s;display:flex;flex-direction:column;align-items:center;gap:4px;}
.quick-btn:active{transform:scale(.95)}
.qb-alloff{background:rgba(255,68,68,.15);color:var(--danger);border:1px solid rgba(255,68,68,.3)}
.qb-allon{background:rgba(0,200,81,.15);color:var(--success);border:1px solid rgba(0,200,81,.3)}
.qb-status{background:rgba(108,99,255,.15);color:var(--primary);border:1px solid rgba(108,99,255,.3)}

/* ── Stats ── */
.stat-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}
.stat-item{background:var(--card2);border-radius:10px;padding:10px;
  text-align:center;border:1px solid var(--border);}
.stat-val{font-size:18px;font-weight:700;margin:4px 0}
.stat-lbl{font-size:9px;color:var(--text2)}
.stat-ok{color:var(--success)}
.stat-err{color:var(--danger)}
.stat-num{color:var(--primary)}

/* ── Forms ── */
.form-group{margin-bottom:12px}
.form-label{font-size:11px;font-weight:700;color:var(--text2);
  margin-bottom:6px;display:block;}
input,select,textarea{
  width:100%;padding:10px 12px;background:var(--card2);
  border:1px solid var(--border);border-radius:10px;
  color:var(--text);font-size:12px;transition:.3s;outline:none;}
input:focus,select:focus,textarea:focus{border-color:var(--primary)}
select option{background:var(--card)}
input[type=checkbox]{width:auto;width:18px;height:18px;
  cursor:pointer;accent-color:var(--primary);}
.checkbox-row{display:flex;align-items:center;gap:8px;padding:10px 12px;
  background:var(--card2);border:1px solid var(--border);border-radius:10px;}
.checkbox-row label{font-size:12px;cursor:pointer;flex:1}

/* ── Buttons ── */
.btn{padding:10px 16px;border:none;border-radius:10px;
  font-size:12px;font-weight:700;cursor:pointer;transition:.3s;
  display:inline-flex;align-items:center;gap:6px;justify-content:center;}
.btn:active{transform:scale(.97)}
.btn-primary{background:linear-gradient(135deg,var(--primary),#a855f7);color:#fff}
.btn-success{background:linear-gradient(135deg,var(--success),#00ff88);color:#000}
.btn-danger{background:linear-gradient(135deg,var(--danger),#ff6b6b);color:#fff}
.btn-warning{background:linear-gradient(135deg,var(--warning),#ffdd00);color:#000}
.btn-ghost{background:var(--card2);color:var(--text);border:1px solid var(--border)}
.btn-sm{padding:6px 12px;font-size:11px}
.btn-block{width:100%}
.btn-row{display:flex;gap:8px;margin-top:12px;flex-wrap:wrap}
.btn-row .btn{flex:1}

/* ── List Items ── */
.list-item{background:var(--card2);border-radius:10px;padding:12px;
  margin-bottom:8px;border:1px solid var(--border);
  display:flex;align-items:center;gap:10px;transition:.3s;}
.list-item:hover{border-color:var(--primary)}
.item-icon{width:36px;height:36px;border-radius:10px;
  background:linear-gradient(135deg,var(--primary),#a855f7);
  display:flex;align-items:center;justify-content:center;
  font-size:16px;flex-shrink:0;}
.item-body{flex:1;min-width:0}
.item-title{font-size:12px;font-weight:700;margin-bottom:3px}
.item-sub{font-size:10px;color:var(--text2);line-height:1.4}
.item-actions{display:flex;gap:6px;flex-shrink:0}

/* ── Tags ── */
.tag{display:inline-flex;align-items:center;gap:3px;
  padding:2px 8px;border-radius:20px;font-size:9px;font-weight:700;margin:2px;}
.tag-single{background:rgba(51,181,229,.2);color:var(--info)}
.tag-double{background:rgba(168,85,247,.2);color:#a855f7}
.tag-long{background:rgba(255,187,51,.2);color:var(--warning)}
.tag-triple{background:rgba(255,68,68,.2);color:var(--danger)}
.tag-scene{background:rgba(108,99,255,.2);color:var(--primary)}
.tag-relay{background:rgba(0,200,81,.2);color:var(--success)}

/* ── Learn Box ── */
.learn-box{background:var(--card2);border-radius:12px;padding:20px;
  text-align:center;border:2px dashed var(--border);
  transition:.5s;margin-bottom:12px;}
.learn-box.active{border-color:var(--primary);
  background:rgba(108,99,255,.08);
  animation:glow .8s ease-in-out infinite alternate;}
@keyframes glow{
  from{box-shadow:0 0 10px rgba(108,99,255,.2)}
  to{box-shadow:0 0 30px rgba(108,99,255,.5)}}
.learn-icon{font-size:40px;margin-bottom:8px;display:block}
.learn-title{font-size:14px;font-weight:700;margin-bottom:4px}
.learn-sub{font-size:11px;color:var(--text2);margin-bottom:12px}
.learn-countdown{font-size:32px;font-weight:700;color:var(--primary);margin:8px 0;}
.code-display{background:var(--bg);border:1px solid var(--primary);
  padding:10px;border-radius:8px;font-family:monospace;font-size:13px;
  color:var(--primary);text-align:center;margin:8px 0;letter-spacing:2px;}

/* ── Step Cards ── */
.step-card{background:var(--bg);border-radius:10px;padding:10px;
  margin-bottom:8px;border:1px solid var(--border);position:relative;}
.step-num{position:absolute;top:-8px;right:10px;
  background:var(--primary);color:#fff;width:20px;height:20px;
  border-radius:50%;font-size:10px;font-weight:700;
  display:flex;align-items:center;justify-content:center;}
.step-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:6px}

/* ── Toast ── */
.toast-container{position:fixed;top:20px;left:50%;transform:translateX(-50%);
  z-index:9999;display:flex;flex-direction:column;gap:6px;
  min-width:200px;max-width:90vw;}
.toast{padding:10px 16px;border-radius:10px;font-size:12px;font-weight:700;
  display:flex;align-items:center;gap:8px;
  animation:slideDown .3s ease;box-shadow:0 4px 20px rgba(0,0,0,.4);}
@keyframes slideDown{from{opacity:0;transform:translateY(-20px)}to{opacity:1;transform:translateY(0)}}
.toast.success{background:var(--success);color:#000}
.toast.error{background:var(--danger);color:#fff}
.toast.info{background:var(--primary);color:#fff}
.toast.warning{background:var(--warning);color:#000}

/* ── Modal ── */
.modal-overlay{position:fixed;inset:0;background:rgba(0,0,0,.7);
  z-index:200;display:flex;align-items:flex-end;
  opacity:0;pointer-events:none;transition:.3s;}
.modal-overlay.open{opacity:1;pointer-events:all}
.modal{width:100%;background:var(--card);border-radius:20px 20px 0 0;
  padding:20px;transform:translateY(100%);transition:.3s;
  max-height:85vh;overflow-y:auto;}
.modal-overlay.open .modal{transform:translateY(0)}
.modal-handle{width:40px;height:4px;background:var(--border);
  border-radius:2px;margin:0 auto 16px;}
.modal-title{font-size:15px;font-weight:700;margin-bottom:16px}

/* ── Confirm ── */
.confirm-overlay{position:fixed;inset:0;background:rgba(0,0,0,.8);
  z-index:300;display:flex;align-items:center;justify-content:center;
  opacity:0;pointer-events:none;transition:.3s;padding:20px;}
.confirm-overlay.open{opacity:1;pointer-events:all}
.confirm-box{background:var(--card);border-radius:16px;padding:24px;
  text-align:center;max-width:300px;width:100%;border:1px solid var(--border);}
.confirm-box h3{font-size:16px;margin-bottom:8px}
.confirm-box p{font-size:12px;color:var(--text2);margin-bottom:20px}
.confirm-btns{display:flex;gap:10px}
.confirm-btns .btn{flex:1}

/* ── Empty ── */
.empty{text-align:center;padding:40px 20px;color:var(--text2);}
.empty .empty-icon{font-size:48px;margin-bottom:12px;display:block;opacity:.5}
.empty p{font-size:12px}

/* ── Dividers & Sections ── */
.divider{height:1px;background:var(--border);margin:16px 0}
.section-title{font-size:11px;font-weight:700;color:var(--text2);
  text-transform:uppercase;letter-spacing:1px;margin-bottom:10px;}

/* ── Settings Rows ── */
.setting-row{display:flex;align-items:center;justify-content:space-between;
  padding:12px 0;border-bottom:1px solid var(--border);}
.setting-row:last-child{border-bottom:none}
.setting-info .setting-title{font-size:12px;font-weight:700}
.setting-info .setting-desc{font-size:10px;color:var(--text2);margin-top:2px}
.toggle-switch{width:44px;height:24px;background:var(--border);
  border-radius:12px;cursor:pointer;position:relative;transition:.3s;flex-shrink:0;}
.toggle-switch.on{background:var(--success)}
.toggle-switch::after{content:'';position:absolute;top:2px;right:2px;
  width:20px;height:20px;background:#fff;border-radius:50%;transition:.3s;}
.toggle-switch.on::after{right:22px}

/* ── Logs ── */
.log-item{background:var(--card2);border-right:3px solid var(--primary);
  border-radius:6px;padding:8px 10px;margin-bottom:6px;
  font-size:11px;line-height:1.6;}
.log-item.warn{border-color:var(--warning)}
.log-item.err{border-color:var(--danger)}
.log-time{color:var(--text2);font-size:9px;font-family:monospace;margin-bottom:2px}
.log-type{display:inline-block;padding:1px 6px;border-radius:4px;
  font-size:9px;font-weight:700;margin-left:4px;}
.log-type.relay{background:rgba(0,200,81,.2);color:var(--success)}
.log-type.scene{background:rgba(108,99,255,.2);color:var(--primary)}
.log-type.rf{background:rgba(255,187,51,.2);color:var(--warning)}
.log-type.sms{background:rgba(51,181,229,.2);color:var(--info)}
.log-type.sys{background:rgba(255,68,68,.2);color:var(--danger)}

/* ── Sensor Cards (Simple) ── */
.sensor-card{
  background:var(--card2);border-radius:12px;padding:14px;
  border:1px solid var(--border);margin-bottom:10px;
  display:flex;align-items:center;gap:12px;
}
.sensor-icon-wrap{
  width:48px;height:48px;border-radius:12px;flex-shrink:0;
  display:flex;align-items:center;justify-content:center;font-size:24px;
  background:linear-gradient(135deg,var(--primary),#a855f7);
}
.sensor-icon-wrap.online{background:linear-gradient(135deg,var(--success),#00ff88)}
.sensor-icon-wrap.offline{background:rgba(255,68,68,.3)}
.sensor-val{font-size:22px;font-weight:700;color:var(--text)}
.sensor-unit{font-size:11px;color:var(--text2)}
.sensor-name-lbl{font-size:12px;font-weight:700}
.sensor-age{font-size:10px;color:var(--text2);margin-top:2px}

/* ── Profile Picker ── */
.profile-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:16px}
.profile-btn{
  background:var(--card2);border:2px solid var(--border);
  border-radius:12px;padding:14px 10px;cursor:pointer;
  text-align:center;transition:.3s;
}
.profile-btn:hover,.profile-btn.selected{border-color:var(--primary);
  background:rgba(108,99,255,.1);}
.profile-btn .pb-icon{font-size:28px;margin-bottom:6px;display:block}
.profile-btn .pb-name{font-size:11px;font-weight:700}
.profile-btn .pb-desc{font-size:9px;color:var(--text2);margin-top:2px}

/* ── Wizard ── */
.wizard-step{
  background:var(--card2);border-radius:12px;
  padding:20px;margin-bottom:12px;
  border:1px solid var(--border);
}
.wizard-num{
  display:inline-flex;align-items:center;justify-content:center;
  width:24px;height:24px;border-radius:50%;
  background:var(--primary);color:#fff;
  font-size:11px;font-weight:700;margin-bottom:10px;
}
.wizard-q{font-size:13px;font-weight:700;margin-bottom:12px}

/* ── Template Cards ── */
.template-card{
  background:var(--card2);border-radius:14px;padding:16px;
  border:1px solid var(--border);margin-bottom:10px;
  cursor:pointer;transition:.3s;display:flex;align-items:center;gap:14px;
}
.template-card:hover{border-color:var(--primary);
  box-shadow:0 0 20px rgba(108,99,255,.2);}
.template-icon{font-size:32px;flex-shrink:0}
.template-name{font-size:13px;font-weight:700}
.template-desc{font-size:11px;color:var(--text2);margin-top:3px}

/* ── Mode Toggle ── */
.mode-bar{
  display:flex;background:var(--card2);border-radius:10px;
  padding:3px;margin-bottom:16px;border:1px solid var(--border);
}
.mode-tab{
  flex:1;padding:8px;border:none;background:none;
  color:var(--text2);font-size:11px;font-weight:700;
  cursor:pointer;border-radius:8px;transition:.3s;
}
.mode-tab.active{background:var(--primary);color:#fff}

/* ── Action Group ── */
.action-group{background:var(--bg);border-radius:10px;padding:10px;
  margin-bottom:8px;border-left:3px solid var(--primary);}
.action-label{font-size:10px;font-weight:700;color:var(--primary);
  margin-bottom:6px;text-transform:uppercase;letter-spacing:1px;}
.action-row{display:flex;gap:6px;align-items:center}
.action-row>*{flex:1}

@media(min-width:480px){
  .relay-grid{grid-template-columns:repeat(4,1fr)}
  .stat-grid{grid-template-columns:repeat(4,1fr)}
  .profile-grid{grid-template-columns:repeat(3,1fr)}
}
</style>
</head>
<body>

<div class="toast-container" id="toastContainer"></div>
<div class="confirm-overlay" id="confirmOverlay">
  <div class="confirm-box">
    <h3>⚠️ تأیید</h3>
    <p id="confirmMsg">آیا مطمئن هستید؟</p>
    <div class="confirm-btns">
      <button class="btn btn-ghost" onclick="confirmResult(false)">لغو</button>
      <button class="btn btn-danger" id="confirmOkBtn" onclick="confirmResult(true)">حذف</button>
    </div>
  </div>
</div>

<!-- ══════════════ HEADER ══════════════ -->
<div class="header">
  <div style="display:flex;align-items:center;justify-content:space-between">
    <div>
      <h1>⚡ Smart Relay Pro_v1.6.1</h1>
      <p id="headerSub">در حال بارگذاری...</p>
    </div>
    <div style="text-align:left">
      <div style="font-size:9px;color:rgba(255,255,255,.7)" id="headerTime">--:--</div>
      <div style="font-size:10px;color:#fff;font-weight:700" id="headerUptime">0s</div>
    </div>
  </div>
  <div class="header-badges" id="headerBadges">
    <div class="hbadge" id="hbWifi">📡 WiFi</div>
    <div class="hbadge" id="hbSim">📱 SIM</div>
    <div class="hbadge" id="hbNet">🌐 شبکه</div>
    <div class="hbadge" id="hbSig">📶 --</div>
  </div>
</div>

<!-- ══════════════ PAGE 0: داشبورد ══════════════ -->
<div class="page active" id="p0">
  <div class="quick-row">
    <button class="quick-btn qb-alloff" onclick="allOff()">
      <span style="font-size:20px">🔴</span>همه خاموش
    </button>
    <button class="quick-btn qb-allon" onclick="allOn()">
      <span style="font-size:20px">🟢</span>همه روشن
    </button>
    <button class="quick-btn qb-status" onclick="doRefresh()">
      <span style="font-size:20px">🔄</span>بروزرسانی
    </button>
  </div>
  <div class="card">
    <div class="card-title"><div class="icon">💡</div>کنترل رله‌ها</div>
    <div class="relay-grid" id="relayGrid"></div>
  </div>
  <div class="card" id="sensorDashCard" style="display:none">
    <div class="card-title"><div class="icon">🌡️</div>سنسورها</div>
    <div id="sensorDash"></div>
  </div>
  <div class="card">
    <div class="card-title"><div class="icon">📊</div>وضعیت سیستم</div>
    <div class="stat-grid" id="statGrid"></div>
  </div>
  <div class="card" id="scenesQuick" style="display:none">
    <div class="card-title"><div class="icon">🎬</div>اجرای سریع سناریو</div>
    <div id="sceneQuickList"></div>
  </div>
</div>

<!-- ══════════════ PAGE 1: سنسورها (ساده) ══════════════ -->
<div class="page" id="p1">

  <!-- mode bar -->
  <div class="mode-bar">
    <button class="mode-tab active" id="sModeSim" onclick="setSensorMode('simple')">
      🟢 ساده
    </button>
    <button class="mode-tab" id="sModeAdv" onclick="setSensorMode('adv')">
      ⚙️ پیشرفته
    </button>
  </div>

  <!-- ── حالت ساده ── -->
  <div id="sSimpleMode">
    <div class="card">
      <div class="card-title"><div class="icon">➕</div>افزودن سنسور جدید</div>

      <!-- مرحله ۱: انتخاب نوع -->
      <div id="sStep1">
        <p style="font-size:12px;color:var(--text2);margin-bottom:14px">
          چه نوع سنسوری دارید؟
        </p>
        <div class="profile-grid" id="profileGrid">
          <!-- با JS پر می‌شه -->
        </div>
      </div>

      <!-- مرحله ۲: نام + یادگیری -->
      <div id="sStep2" style="display:none">
        <div style="display:flex;align-items:center;gap:10px;margin-bottom:14px">
          <span id="sStep2Icon" style="font-size:32px"></span>
          <div>
            <div id="sStep2Title" style="font-size:14px;font-weight:700"></div>
            <div id="sStep2Desc" style="font-size:11px;color:var(--text2)"></div>
          </div>
        </div>
        <div class="form-group">
          <label class="form-label">اسم این سنسور را وارد کنید:</label>
          <input type="text" id="sSimpleName" placeholder="مثال: دمای انبار">
        </div>
        <div id="sLearnArea">
          <div class="learn-box" id="sSimpleLearnBox">
            <span class="learn-icon">📡</span>
            <div class="learn-title">آماده یادگیری</div>
            <div class="learn-sub">دکمه زیر را بزنید، سپس سنسور را یک‌بار ارسال کنید</div>
            <button class="btn btn-primary" onclick="sSimpleLearn()">🎓 شروع یادگیری</button>
          </div>
          <div id="sSimpleLearnActive" style="display:none">
            <div class="learn-box active">
              <span class="learn-icon">🔊</span>
              <div class="learn-title">در حال دریافت...</div>
              <div class="learn-sub">سنسور را ارسال کنید</div>
              <div class="learn-countdown" id="sSimpleCD">30</div>
              <div class="learn-sub">ثانیه باقیمانده</div>
            </div>
            <button class="btn btn-ghost btn-block" onclick="sSimpleCancel()">❌ لغو</button>
          </div>
          <div id="sSimpleLearnDone" style="display:none">
            <div class="code-display" id="sSimpleCode">--</div>
            <p style="font-size:11px;color:var(--success);text-align:center;margin-bottom:12px">
              ✅ سنسور شناسایی شد!
            </p>
          </div>
        </div>
        <div class="btn-row">
          <button class="btn btn-ghost" onclick="sBackToStep1()">← برگشت</button>
          <button class="btn btn-success" id="sSaveSimpleBtn" onclick="sSaveSimple()" disabled>💾 ذخیره</button>
        </div>
      </div>
    </div>

    <!-- لیست سنسورهای ثبت‌شده -->
    <div class="card">
      <div class="card-title">
        <div class="icon">📋</div>سنسورهای من
        <span style="margin-right:auto;font-size:10px;color:var(--text2)" id="sensorCounter">۰</span>
        <button class="btn btn-ghost btn-sm" onclick="loadSensorsPage()">🔄</button>
      </div>
      <div id="sensorList">
        <div class="empty"><span class="empty-icon">🌡️</span><p>هنوز سنسوری ثبت نشده</p></div>
      </div>
    </div>
  </div>

  <!-- ── حالت پیشرفته ── -->
  <div id="sAdvMode" style="display:none">
    <div class="card">
      <div class="card-title"><div class="icon">🌡️</div>یادگیری سنسور (پیشرفته)</div>
      <div id="sLearnIdle">
        <p style="font-size:11px;color:var(--text2);margin-bottom:12px">
          کنترل کامل روی پارامترهای سنسور — برای کاربران حرفه‌ای.
        </p>
        <button class="btn btn-primary btn-block" onclick="sLearnStart()">📊 شروع یادگیری</button>
      </div>
      <div id="sLearnBox" style="display:none">
        <div class="learn-box active">
          <span class="learn-icon">📡</span>
          <div class="learn-title">در حال یادگیری...</div>
          <div class="learn-sub">سنسور را ارسال کنید</div>
          <div class="learn-countdown" id="sLearnCD">30</div>
          <div class="learn-sub">ثانیه باقیمانده</div>
        </div>
        <button class="btn btn-ghost btn-block" onclick="sLearnCancel()">❌ لغو</button>
      </div>
      <div id="sLearnCfg" style="display:none">
        <div class="code-display" id="sLearnInfo">--</div>
        <div class="form-group">
          <label class="form-label">نام سنسور:</label>
          <input type="text" id="sNewName" placeholder="مثال: دمای اتاق">
        </div>
        <div class="form-group">
          <label class="form-label">نوع مقدار:</label>
          <select id="sNewType">
            <option value="0">درصد (0-100%)</option>
            <option value="1">دما (°C)</option>
            <option value="2">رطوبت (%RH)</option>
            <option value="3">فاصله (cm)</option>
            <option value="4">ولتاژ</option>
            <option value="5">خام/عام</option>
          </select>
        </div>
        <div class="step-grid">
          <div class="form-group" style="margin:0">
            <label class="form-label">بیت مقدار:</label>
            <input type="number" id="sNewValueBits" value="8" min="1" max="32">
          </div>
          <div class="form-group" style="margin:0">
            <label class="form-label">ضریب (Scale):</label>
            <input type="number" id="sNewScale" value="1.0" step="0.01">
          </div>
        </div>
        <div class="form-group" style="margin-top:8px">
          <label class="form-label">جابجایی (Offset):</label>
          <input type="number" id="sNewOffset" value="0" step="0.1">
        </div>
        <div class="btn-row">
          <button class="btn btn-success" onclick="sLearnSave()">💾 ذخیره</button>
          <button class="btn btn-ghost" onclick="sLearnReset()">❌ لغو</button>
        </div>
      </div>
    </div>
    <div class="card">
      <div class="card-title">
        <div class="icon">📋</div>سنسورهای ثبت‌شده (پیشرفته)
        <button class="btn btn-ghost btn-sm" onclick="loadSensorsPage()" style="margin-right:auto">🔄</button>
      </div>
      <div id="sensorListAdv">
        <div class="empty"><span class="empty-icon">🌡️</span><p>هیچ سنسوری ثبت نشده</p></div>
      </div>
    </div>
  </div>

  <!-- modal ویرایش سنسور -->
  <div class="modal-overlay" id="sEditModal">
    <div class="modal">
      <div class="modal-handle"></div>
      <div class="modal-title">⚙️ تنظیمات سنسور</div>
      <input type="hidden" id="sEditId">
      <div class="form-group">
        <label class="form-label">نام:</label>
        <input type="text" id="sEditName">
      </div>
      <div class="form-group">
        <label class="form-label">نوع:</label>
        <select id="sEditType">
          <option value="0">درصد</option><option value="1">دما °C</option>
          <option value="2">رطوبت</option><option value="3">فاصله</option>
          <option value="4">ولتاژ</option><option value="5">خام</option>
        </select>
      </div>
      <div class="step-grid">
        <div class="form-group" style="margin:0">
          <label class="form-label">بیت مقدار:</label>
          <input type="number" id="sEditValueBits" min="1" max="32">
        </div>
        <div class="form-group" style="margin:0">
          <label class="form-label">ضریب:</label>
          <input type="number" id="sEditScale" step="0.01">
        </div>
      </div>
      <div class="form-group" style="margin-top:8px">
        <label class="form-label">Offset:</label>
        <input type="number" id="sEditOffset" step="0.1">
      </div>
      <div class="btn-row">
        <button class="btn btn-success" onclick="sEditSave()">💾 ذخیره</button>
        <button class="btn btn-ghost" onclick="closeModal('sEditModal')">❌</button>
      </div>
    </div>
  </div>
</div>

<!-- ══════════════ PAGE 2: اتوماسیون ══════════════ -->
<div class="page" id="p2">

  <div class="mode-bar">
    <button class="mode-tab active" id="aModeSim" onclick="setAutoMode('simple')">
      🟢 ساده
    </button>
    <button class="mode-tab" id="aModeAdv" onclick="setAutoMode('adv')">
      ⚙️ پیشرفته
    </button>
  </div>

  <!-- ── حالت ساده ── -->
  <div id="aSimpleMode">
    <div class="card">
      <div class="card-title"><div class="icon">🤖</div>افزودن اتوماسیون</div>
      <p style="font-size:12px;color:var(--text2);margin-bottom:14px">
        یکی از موارد زیر را انتخاب کنید:
      </p>
      <div id="templateList">
        <div class="template-card" onclick="openTemplate('irrigation')">
          <span class="template-icon">🌱</span>
          <div>
            <div class="template-name">آبیاری کشاورزی</div>
            <div class="template-desc">روشن کردن پمپ در ساعت مشخص — بدون نیاز به اینترنت</div>
          </div>
        </div>
        <div class="template-card" onclick="openTemplate('thermostat')">
          <span class="template-icon">🌡️</span>
          <div>
            <div class="template-name">کنترل دما</div>
            <div class="template-desc">روشن/خاموش خودکار بر اساس سنسور دما</div>
          </div>
        </div>
        <div class="template-card" onclick="openTemplate('soilmoisture')">
          <span class="template-icon">💧</span>
          <div>
            <div class="template-name">رطوبت خاک</div>
            <div class="template-desc">آبیاری خودکار وقتی خاک خشک شد</div>
          </div>
        </div>
        <div class="template-card" onclick="openTemplate('level')">
          <span class="template-icon">🪣</span>
          <div>
            <div class="template-name">سطح مایع</div>
            <div class="template-desc">هشدار یا خاموش کردن وقتی تانک پر/خالی شد</div>
          </div>
        </div>
        <div class="template-card" onclick="openTemplate('door')">
          <span class="template-icon">🚪</span>
          <div>
            <div class="template-name">در و پنجره</div>
            <div class="template-desc">هشدار SMS یا روشن کردن چراغ وقتی در باز شد</div>
          </div>
        </div>
        <div class="template-card" onclick="openTemplate('timedrelay')">
          <span class="template-icon">⏰</span>
          <div>
            <div class="template-name">زمان‌بندی ساده</div>
            <div class="template-desc">روشن و خاموش خودکار رله در ساعت مشخص</div>
          </div>
        </div>
      </div>
    </div>

    <!-- لیست اتوماسیون‌های ساده -->
    <div id="autoSimpleList"></div>
  </div>

  <!-- ── حالت پیشرفته ── -->
  <div id="aAdvMode" style="display:none">
    <div class="card">
      <div class="card-title">
        <div class="icon">🤖</div>موتور اتوماسیون
        <span style="margin-right:auto;font-size:10px;color:var(--text2)" id="autoCounter">۰</span>
      </div>
      <button class="btn btn-primary btn-block" onclick="openAutoModal()">➕ اتوماسیون جدید</button>
    </div>
    <div id="autoList">
      <div class="empty"><span class="empty-icon">🤖</span><p>اتوماسیونی تعریف نشده</p></div>
    </div>
  </div>

  <!-- Modal: wizard template -->
  <div class="modal-overlay" id="templateModal">
    <div class="modal">
      <div class="modal-handle"></div>
      <div class="modal-title" id="tmplTitle">🌱 آبیاری کشاورزی</div>
      <div id="tmplBody"></div>
      <div class="btn-row" style="margin-top:16px">
        <button class="btn btn-success" onclick="saveTemplate()">💾 ذخیره</button>
        <button class="btn btn-ghost" onclick="closeModal('templateModal')">❌ لغو</button>
      </div>
    </div>
  </div>

  <!-- Modal: اتوماسیون پیشرفته -->
  <div class="modal-overlay" id="autoModal">
    <div class="modal" style="max-height:92vh">
      <div class="modal-handle"></div>
      <div class="modal-title">🤖 اتوماسیون جدید / ویرایش</div>
      <input type="hidden" id="autoEditId" value="-1">
      <div class="form-group">
        <label class="form-label">نام اتوماسیون:</label>
        <input type="text" id="autoName" placeholder="مثال: ترموستات اتاق">
      </div>
      <div class="form-group">
        <label class="form-label">اپراتور بین شرط‌ها:</label>
        <select id="autoLogicOp">
          <option value="0">AND — همه شرط‌ها باید برقرار باشند</option>
          <option value="1">OR — حداقل یک شرط</option>
        </select>
      </div>
      <div class="divider"></div>
      <div class="section-title">📋 شرط‌ها (IF)</div>
      <div id="autoCondList"></div>
      <button class="btn btn-ghost btn-block" onclick="addAutoCond()" style="margin-bottom:12px">➕ افزودن شرط</button>
      <div class="divider"></div>
      <div class="section-title">🌡️ هیسترزیس (اختیاری)</div>
      <div class="checkbox-row" style="margin-bottom:8px">
        <input type="checkbox" id="autoHystEnabled" style="width:18px;height:18px"
          onchange="document.getElementById('autoHystFields').style.display=this.checked?'block':'none'">
        <label for="autoHystEnabled">فعال‌سازی هیسترزیس</label>
      </div>
      <div id="autoHystFields" style="display:none">
        <div class="step-grid">
          <div class="form-group" style="margin:0">
            <label class="form-label">سنسور:</label>
            <select id="autoHystSensor"></select>
          </div>
          <div class="form-group" style="margin:0">
            <label class="form-label">رله:</label>
            <select id="autoHystRelay">
              <option value="0">رله ۱</option><option value="1">رله ۲</option>
              <option value="2">رله ۳</option><option value="3">رله ۴</option>
            </select>
          </div>
        </div>
        <div class="step-grid" style="margin-top:8px">
          <div class="form-group" style="margin:0">
            <label class="form-label">آستانه روشن:</label>
            <input type="number" id="autoHystOn" value="30" step="0.1">
          </div>
          <div class="form-group" style="margin:0">
            <label class="form-label">آستانه خاموش:</label>
            <input type="number" id="autoHystOff" value="27" step="0.1">
          </div>
        </div>
      </div>
      <div class="divider"></div>
      <div class="section-title">⚡ اعمال (THEN)</div>
      <div id="autoActList"></div>
      <button class="btn btn-ghost btn-block" onclick="addAutoAct()" style="margin-bottom:12px">➕ افزودن عمل</button>
      <div class="divider"></div>
      <div class="section-title">⏳ Cooldown</div>
      <div class="checkbox-row" style="margin-bottom:8px">
        <input type="checkbox" id="autoCoolEnabled" style="width:18px;height:18px"
          onchange="document.getElementById('autoCoolFields').style.display=this.checked?'block':'none'">
        <label for="autoCoolEnabled">فعال‌سازی Cooldown</label>
      </div>
      <div id="autoCoolFields" style="display:none">
        <div class="form-group">
          <label class="form-label">تأخیر بین دو trigger (دقیقه):</label>
          <input type="number" id="autoCoolMin" value="5" min="1" max="1440">
        </div>
      </div>
      <div class="btn-row" style="margin-top:16px">
        <button class="btn btn-success" onclick="saveAutomation()">💾 ذخیره</button>
        <button class="btn btn-ghost" onclick="closeModal('autoModal')">❌ لغو</button>
      </div>
    </div>
  </div>
</div>

<!-- ══════════════ PAGE 3: ریموت ══════════════ -->
<div class="page" id="p3">
  <div class="card">
    <div id="learnBox" class="learn-box">
      <span class="learn-icon">📡</span>
      <div class="learn-title">یادگیری دکمه ریموت</div>
      <div class="learn-sub">دکمه را فشار دهید تا کد دریافت شود</div>
      <button class="btn btn-primary" onclick="startLearn()">🎓 شروع یادگیری</button>
    </div>
    <div id="learnActive" style="display:none">
      <div class="learn-box active">
        <span class="learn-icon">🔊</span>
        <div class="learn-title">در حال دریافت...</div>
        <div class="learn-sub">دکمه ریموت را فشار دهید</div>
        <div class="learn-countdown" id="learnCountdown">20</div>
        <div class="learn-sub">ثانیه باقیمانده</div>
      </div>
      <button class="btn btn-ghost btn-block" onclick="cancelLearn()">❌ لغو</button>
    </div>
    <div id="learnResult" style="display:none">
      <div class="code-display" id="learnCode">--</div>
      <div class="form-group">
        <label class="form-label">نام دکمه:</label>
        <input type="text" id="btnName" placeholder="مثال: چراغ پذیرایی">
      </div>
      <div class="section-title">عملکردها</div>
      <div class="action-group">
        <div class="action-label">🖱️ تک کلیک</div>
        <div class="action-row">
          <select id="sa" onchange="updateActionUI('s')">
            <option value="0">— بدون عملکرد —</option>
            <option value="2">Toggle رله</option>
            <option value="3">روشن کردن رله</option>
            <option value="4">خاموش کردن رله</option>
            <option value="5">خاموش کردن همه</option>
            <option value="1">اجرای سناریو</option>
          </select>
          <select id="st" class="target-sel"></select>
        </div>
      </div>
      <div class="action-group">
        <div class="action-label">🖱️🖱️ دوبار کلیک</div>
        <div class="action-row">
          <select id="da" onchange="updateActionUI('d')">
            <option value="0">— بدون عملکرد —</option>
            <option value="2">Toggle رله</option>
            <option value="3">روشن کردن رله</option>
            <option value="4">خاموش کردن رله</option>
            <option value="5">خاموش کردن همه</option>
            <option value="1">اجرای سناریو</option>
          </select>
          <select id="dt" class="target-sel"></select>
        </div>
      </div>
      <div class="action-group">
        <div class="action-label">⏱️ نگه‌داشتن</div>
        <div class="action-row">
          <select id="la" onchange="updateActionUI('l')">
            <option value="0">— بدون عملکرد —</option>
            <option value="2">Toggle رله</option>
            <option value="3">روشن کردن رله</option>
            <option value="4">خاموش کردن رله</option>
            <option value="5">خاموش کردن همه</option>
            <option value="1">اجرای سناریو</option>
          </select>
          <select id="lt" class="target-sel"></select>
        </div>
      </div>
      <div class="btn-row">
        <button class="btn btn-success" onclick="saveBtn()">💾 ذخیره</button>
        <button class="btn btn-ghost" onclick="cancelLearn()">❌ لغو</button>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="card-title">
      <div class="icon">📋</div>دکمه‌های ثبت‌شده
      <span style="margin-right:auto;font-size:10px;color:var(--text2)" id="rfCounter">0</span>
    </div>
    <div id="rfList">
      <div class="empty"><span class="empty-icon">📡</span><p>هیچ دکمه‌ای ثبت نشده</p></div>
    </div>
  </div>
</div>

<!-- ══════════════ PAGE 4: سناریو ══════════════ -->
<div class="page" id="p4">
  <div class="card">
    <div class="card-title">
      <div class="icon">🎬</div>سناریوها
      <span style="margin-right:auto;font-size:10px;color:var(--text2)" id="sceneCounter">۰</span>
    </div>
    <button class="btn btn-primary btn-block" onclick="openSceneModal()">➕ سناریو جدید</button>
  </div>
  <div id="sceneList">
    <div class="empty"><span class="empty-icon">🎬</span><p>سناریویی تعریف نشده</p></div>
  </div>

  <div class="modal-overlay" id="sceneModal">
    <div class="modal">
      <div class="modal-handle"></div>
      <div class="modal-title">🎬 سناریو جدید</div>
      <div class="form-group">
        <label class="form-label">نام سناریو:</label>
        <input type="text" id="sceneName" placeholder="مثال: آبیاری باغچه">
      </div>
      <div class="checkbox-row">
        <input type="checkbox" id="sceneSeq" style="width:18px;height:18px">
        <label for="sceneSeq">اجرای مرحله‌ای</label>
      </div>
      <div class="checkbox-row" style="margin-top:8px">
        <input type="checkbox" id="sceneTimeEnabled" style="width:18px;height:18px"
          onchange="document.getElementById('sceneTimePicker').style.display=this.checked?'block':'none'">
        <label for="sceneTimeEnabled">⏰ اجرای خودکار در ساعت مشخص</label>
      </div>
      <div id="sceneTimePicker" style="display:none;margin-top:8px">
        <div class="step-grid">
          <div class="form-group" style="margin:0">
            <label class="form-label">ساعت (0-23):</label>
            <input type="number" id="sceneTriggerHour" min="0" max="23" value="0">
          </div>
          <div class="form-group" style="margin:0">
            <label class="form-label">دقیقه (0-59):</label>
            <input type="number" id="sceneTriggerMinute" min="0" max="59" value="0">
          </div>
        </div>
        <div class="form-group" style="margin-top:10px">
          <label class="form-label">روزهای هفته:</label>
          <div id="weekdayPicker" style="display:flex;flex-wrap:wrap;gap:6px;margin-top:6px">
            <label style="display:flex;align-items:center;gap:4px;font-size:12px;cursor:pointer"><input type="checkbox" class="wday" value="0" checked style="width:16px;height:16px;accent-color:var(--primary)">ی</label>
            <label style="display:flex;align-items:center;gap:4px;font-size:12px;cursor:pointer"><input type="checkbox" class="wday" value="1" checked style="width:16px;height:16px;accent-color:var(--primary)">د</label>
            <label style="display:flex;align-items:center;gap:4px;font-size:12px;cursor:pointer"><input type="checkbox" class="wday" value="2" checked style="width:16px;height:16px;accent-color:var(--primary)">س</label>
            <label style="display:flex;align-items:center;gap:4px;font-size:12px;cursor:pointer"><input type="checkbox" class="wday" value="3" checked style="width:16px;height:16px;accent-color:var(--primary)">چ</label>
            <label style="display:flex;align-items:center;gap:4px;font-size:12px;cursor:pointer"><input type="checkbox" class="wday" value="4" checked style="width:16px;height:16px;accent-color:var(--primary)">پ</label>
            <label style="display:flex;align-items:center;gap:4px;font-size:12px;cursor:pointer"><input type="checkbox" class="wday" value="5" checked style="width:16px;height:16px;accent-color:var(--primary)">ج</label>
            <label style="display:flex;align-items:center;gap:4px;font-size:12px;cursor:pointer"><input type="checkbox" class="wday" value="6" checked style="width:16px;height:16px;accent-color:var(--primary)">ش</label>
          </div>
        </div>
        <div class="form-group" style="margin-top:8px">
          <label class="form-label">🔁 تکرار هر چند دقیقه (0=یک‌بار در روز):</label>
          <input type="number" id="sceneRepeatInterval" min="0" max="10080" value="0">
        </div>
      </div>
      <div class="divider"></div>
      <div class="section-title">مراحل اجرا</div>
      <div id="stepList"></div>
      <button class="btn btn-ghost btn-block" onclick="addStep()" style="margin-bottom:12px">➕ افزودن مرحله</button>
      <div class="btn-row">
        <button class="btn btn-success" onclick="saveScene()">💾 ذخیره</button>
        <button class="btn btn-ghost" onclick="closeModal('sceneModal')">❌ لغو</button>
      </div>
    </div>
  </div>
</div>

<!-- ══════════════ PAGE 5: ترکیب RF ══════════════ -->
<div class="page" id="p5">
  <div class="card">
    <div class="card-title"><div class="icon">🔗</div>ترکیب دکمه‌ها</div>
    <p style="font-size:11px;color:var(--text2);margin-bottom:12px">
      دو دکمه که در عرض ۸۰۰ms فشرده شوند، یک عملکرد مشترک انجام می‌دهند.
    </p>
    <button class="btn btn-primary btn-block" onclick="openComboModal()">➕ ترکیب جدید</button>
  </div>
  <div id="comboList">
    <div class="empty"><span class="empty-icon">🔗</span><p>ترکیبی تعریف نشده</p></div>
  </div>

  <div class="modal-overlay" id="comboModal">
    <div class="modal">
      <div class="modal-handle"></div>
      <div class="modal-title">➕ ترکیب جدید</div>
      <div class="form-group">
        <label class="form-label">نام ترکیب:</label>
        <input type="text" id="comboName" placeholder="مثال: همه خاموش">
      </div>
      <div class="form-group">
        <label class="form-label">دکمه اول:</label>
        <select id="comboCode1"></select>
      </div>
      <div class="form-group">
        <label class="form-label">دکمه دوم:</label>
        <select id="comboCode2"></select>
      </div>
      <div class="form-group">
        <label class="form-label">عملکرد:</label>
        <select id="comboAction" onchange="updateComboUI()">
          <option value="2">Toggle رله</option>
          <option value="3">روشن کردن رله</option>
          <option value="4">خاموش کردن رله</option>
          <option value="5">خاموش کردن همه</option>
          <option value="1">اجرای سناریو</option>
        </select>
      </div>
      <div class="form-group" id="comboTargetGroup">
        <label class="form-label">هدف:</label>
        <select id="comboTarget"></select>
      </div>
      <div class="btn-row">
        <button class="btn btn-success" onclick="saveCombo()">💾 ذخیره</button>
        <button class="btn btn-ghost" onclick="closeModal('comboModal')">❌ لغو</button>
      </div>
    </div>
  </div>
</div>

<!-- ══════════════ PAGE 6: تنظیمات ══════════════ -->
<div class="page" id="p6">
  <div class="card">
    <div class="card-title"><div class="icon">📱</div>مدیریت GSM</div>
    <div class="btn-row">
      <button class="btn btn-warning" onclick="gsmSoftReset()">🔄 ریست نرم</button>
      <button class="btn btn-danger" onclick="gsmHardReset()">⚡ ریست سخت</button>
    </div>
  </div>
  <div class="card">
    <div class="card-title"><div class="icon">⚙️</div>تنظیمات رله‌ها</div>
    <div id="relaySettings"></div>
  </div>
  <div class="card">
    <div class="card-title"><div class="icon">📱</div>شماره‌های مجاز
      <span style="margin-right:auto;font-size:10px;color:var(--text2)" id="phoneCounter">۰</span>
    </div>
    <p style="font-size:11px;color:var(--text2);margin-bottom:12px">
      اگر لیست خالی باشد، همه شماره‌ها مجاز هستند.
    </p>
    <div style="display:flex;gap:8px;margin-bottom:12px">
      <input type="text" id="newPhone" placeholder="+989123456789" style="flex:1">
      <button class="btn btn-primary" onclick="addPhone()">➕</button>
    </div>
    <div id="phoneList">
      <div class="empty"><span class="empty-icon">📱</span><p>همه شماره‌ها مجاز</p></div>
    </div>
  </div>
  <div class="card">
    <div class="card-title"><div class="icon">🗄️</div>مدیریت داده</div>
    <div style="display:flex;flex-direction:column;gap:8px">
      <button class="btn btn-danger btn-block" onclick="clearAllData()">🗑️ پاک کردن همه تنظیمات</button>
    </div>
  </div>
  <div class="card">
    <div class="card-title">
      <div class="icon">📋</div>لاگ رویدادها
      <span style="margin-right:auto;font-size:10px;color:var(--text2)" id="logCounter">۰</span>
    </div>
    <div style="margin-bottom:12px;display:flex;gap:8px">
      <button class="btn btn-ghost" onclick="loadLogs()" style="flex:1">🔄</button>
      <button class="btn btn-danger" onclick="clearLogs()">🗑️</button>
    </div>
    <div id="logList" style="max-height:400px;overflow-y:auto">
      <div class="empty"><span class="empty-icon">📋</span><p>لاگی وجود ندارد</p></div>
    </div>
  </div>
  <div class="card">
    <div class="card-title"><div class="icon">ℹ️</div>اطلاعات سیستم</div>
    <div id="sysInfo" style="font-size:11px;color:var(--text2);line-height:2"></div>
  </div>
</div>

<!-- ══════════════ PAGE 7: WiFi / OTA ══════════════ -->
<div class="page" id="p7">
  <div class="card">
    <div class="card-title"><div class="icon">📶</div>اتصال به WiFi</div>
    <p style="font-size:11px;color:var(--text2);margin-bottom:12px">
      پس از اتصال، بروزرسانی خودکار بررسی می‌شود.
    </p>
    <div class="form-group">
      <label class="form-label">نام شبکه (SSID):</label>
      <input type="text" id="wifiSSID" placeholder="مثال: HomeWifi">
    </div>
    <div class="form-group">
      <label class="form-label">رمز عبور:</label>
      <input type="password" id="wifiPass" placeholder="رمز شبکه">
    </div>
    <div id="wifiStatus" style="font-size:11px;color:var(--text2);margin-bottom:10px;min-height:16px"></div>
    <button class="btn btn-primary btn-block" id="wifiConnectBtn" onclick="connectWifi()">
      🔗 اتصال و بررسی بروزرسانی
    </button>
  </div>
</div>

<!-- ══════════════ BOTTOM NAV ══════════════ -->
<nav class="bottom-nav">
  <div class="nav-indicator" id="navIndicator"></div>
  <button class="nav-btn active" id="nb0" onclick="showPage(0)">
    <span class="icon">🏠</span>خانه
  </button>
  <button class="nav-btn" id="nb1" onclick="showPage(1)">
    <span class="icon">🌡️</span>سنسور
  </button>
  <button class="nav-btn" id="nb2" onclick="showPage(2)">
    <span class="icon">🤖</span>اتوماسیون
  </button>
  <button class="nav-btn" id="nb3" onclick="showPage(3)">
    <span class="icon">📡</span>ریموت
  </button>
  <button class="nav-btn" id="nb4" onclick="showPage(4)">
    <span class="icon">🎬</span>سناریو
  </button>
  <button class="nav-btn" id="nb5" onclick="showPage(5)">
    <span class="icon">🔗</span>ترکیب
  </button>
  <button class="nav-btn" id="nb6" onclick="showPage(6)">
    <span class="icon">⚙️</span>تنظیمات
  </button>
  <button class="nav-btn" id="nb7" onclick="showPage(7)">
    <span class="icon">📶</span>WiFi
  </button>
</nav>

<script>
// ══════════════════════════════════════════════
//  STATE
// ══════════════════════════════════════════════
const S = {
  currentPage: 0,
  startTime: Date.now(),
  learnCode: 0, learnProto: 0, learnBits: 0,
  learnTimer: null,
  stepCount: 0,
  sensorMode: 'simple',
  autoMode: 'simple',
  selectedProfile: null,
  simpleLearnData: null,
  simpleLearnTimer: null,
  simpleLearnCD: 30,
  currentTemplate: null
};

const relayNames = ['رله ۱','رله ۲','رله ۳','رله ۴'];
const actionLabels = ['—','سناریو','Toggle','روشن','خاموش','همه خاموش'];

// ── پروفایل سنسورها ──────────────────────────────────────────────
const SENSOR_PROFILES = [
  {
    id:'temp', icon:'🌡️', name:'دما', desc:'سنسور دما (°C)',
    valueType:1, valueBits:8, scale:1.0, offset:0,
    unit:'°C', baseMask:0xFFFF00,
    hint:'مثال: DHT11, DS18B20'
  },
  {
    id:'humidity', icon:'💦', name:'رطوبت هوا', desc:'رطوبت محیط (%)',
    valueType:2, valueBits:8, scale:1.0, offset:0,
    unit:'%RH', baseMask:0xFFFF00,
    hint:'مثال: DHT11'
  },
  {
    id:'soilmoist', icon:'🌱', name:'رطوبت خاک', desc:'خشکی/مرطوبی خاک (%)',
    valueType:0, valueBits:8, scale:1.0, offset:0,
    unit:'%', baseMask:0xFFFF00,
    hint:'سنسور capacitive یا resistive'
  },
  {
    id:'level', icon:'🪣', name:'سطح مایع', desc:'ارتفاع مایع (cm)',
    valueType:3, valueBits:8, scale:1.0, offset:0,
    unit:'cm', baseMask:0xFFFF00,
    hint:'سنسور اولتراسونیک یا float'
  },
  {
    id:'door', icon:'🚪', name:'در / پنجره', desc:'باز یا بسته',
    valueType:5, valueBits:1, scale:1.0, offset:0,
    unit:'', baseMask:0xFFFFFE,
    hint:'مگنت یا میکروسوییچ'
  },
  {
    id:'voltage', icon:'⚡', name:'ولتاژ', desc:'سطح ولتاژ (V)',
    valueType:4, valueBits:8, scale:0.1, offset:0,
    unit:'V', baseMask:0xFFFF00,
    hint:'ولتاژ باتری یا شبکه'
  }
];

// ══════════════════════════════════════════════
//  UTILITIES
// ══════════════════════════════════════════════
function toast(msg, type='success', dur=2500) {
  const icons={success:'✅',error:'❌',info:'ℹ️',warning:'⚠️'};
  const el=document.createElement('div');
  el.className=`toast ${type}`;
  el.innerHTML=`<span>${icons[type]||'•'}</span><span>${msg}</span>`;
  document.getElementById('toastContainer').appendChild(el);
  setTimeout(()=>el.style.opacity='0',dur-300);
  setTimeout(()=>el.remove(),dur);
}

let confirmCb=null;
function showConfirm(msg,okText='حذف',cb){
  confirmCb=cb;
  document.getElementById('confirmMsg').textContent=msg;
  document.getElementById('confirmOkBtn').textContent=okText;
  document.getElementById('confirmOverlay').classList.add('open');
}
function confirmResult(ok){
  document.getElementById('confirmOverlay').classList.remove('open');
  if(ok&&confirmCb)confirmCb();
  confirmCb=null;
}
function openModal(id){document.getElementById(id).classList.add('open')}
function closeModal(id){document.getElementById(id).classList.remove('open')}

document.querySelectorAll('.modal-overlay').forEach(el=>{
  el.addEventListener('click',e=>{if(e.target===el)el.classList.remove('open')});
});

async function api(url,method='GET',body=null){
  try{
    const opts={method,headers:{},credentials:'same-origin'};
    if(body!==null){opts.headers['Content-Type']='application/json';opts.body=JSON.stringify(body);}
    const r=await fetch(url,opts);
    if(!r.ok)throw new Error(r.statusText);
    const ct=r.headers.get('content-type')||'';
    return ct.includes('json')?r.json():r.text();
  }catch(e){toast('خطا: '+e.message,'error');throw e;}
}

// ══════════════════════════════════════════════
//  NAVIGATION
// ══════════════════════════════════════════════
window.showPage=function(n){
  for(let i=0;i<8;i++){
    document.getElementById('p'+i)?.classList.remove('active');
    document.getElementById('nb'+i)?.classList.remove('active');
  }
  document.getElementById('p'+n)?.classList.add('active');
  document.getElementById('nb'+n)?.classList.add('active');
  S.currentPage=n;
  const w=100/8;
  document.getElementById('navIndicator').style.cssText=`width:${w}%;right:${n*w}%;`;
  if(n===0)doRefresh();
  if(n===1){buildProfileGrid();loadSensorsPage();}
  if(n===2)loadAutomations();
  if(n===3)loadRF();
  if(n===4)loadScenes();
  if(n===5)loadCombos();
  if(n===6)loadSettings();
  if(n===7){}
};

// ══════════════════════════════════════════════
//  DASHBOARD
// ══════════════════════════════════════════════
async function doRefresh(){
  try{
    const d=await api('/api/status');
    document.getElementById('hbWifi').className=`hbadge ${d.wifi?'ok':'err'}`;
    document.getElementById('hbWifi').textContent=`📡 ${d.wifi?'WiFi ✓':'WiFi ✗'}`;
    document.getElementById('hbSim').className=`hbadge ${d.sim800?'ok':'err'}`;
    document.getElementById('hbSim').textContent=`📱 ${d.sim800?'SIM ✓':'SIM ✗'}`;
    document.getElementById('hbNet').className=`hbadge ${d.network?'ok':'err'}`;
    document.getElementById('hbNet').textContent=`🌐 ${d.network?'اتصال ✓':'قطع ✗'}`;
    document.getElementById('hbSig').textContent=`📶 ${d.signal}`;
    document.getElementById('headerSub').textContent=
      d.clockValid?(d.date+' | '+d.time):'Smart Relay Pro';

    let rgHtml='';
    for(const x of d.relays){
      const rem=x.remaining||0;
      rgHtml+=`
      <div class="relay-card ${x.active?'on':''}" onclick="toggleRelayUI(${x.id-1})">
        <div class="relay-name">${relayNames[x.id-1]}</div>
        <div class="relay-status">
          <div class="dot ${x.active?'on':''}"></div>
          <span>${x.active?'روشن':'خاموش'}</span>
        </div>
        <button class="relay-toggle ${x.active?'on-btn':'off-btn'}"
          onclick="event.stopPropagation();toggleRelayUI(${x.id-1})">
          ${x.active?'⬛ خاموش':'▶️ روشن'}
        </button>
        ${rem>0?`<div class="relay-timer">⏱️ ${rem}s</div>`:''}
      </div>`;
    }
    document.getElementById('relayGrid').innerHTML=rgHtml;

    document.getElementById('statGrid').innerHTML=`
    <div class="stat-item"><div class="stat-lbl">📡 ریموت</div>
      <div class="stat-val stat-num">${d.rfCount}</div></div>
    <div class="stat-item"><div class="stat-lbl">🎬 سناریو</div>
      <div class="stat-val stat-num">${d.sceneCount}</div></div>
    <div class="stat-item"><div class="stat-lbl">🤖 اتوماسیون</div>
      <div class="stat-val stat-num">${d.automationCount||0}</div></div>
    <div class="stat-item"><div class="stat-lbl">📶 سیگنال</div>
      <div class="stat-val stat-num">${d.signal}</div></div>
    <div class="stat-item"><div class="stat-lbl">🌡️ سنسور</div>
      <div class="stat-val stat-num">${d.sensorCount||0}</div></div>
    <div class="stat-item"><div class="stat-lbl">🔋 حافظه</div>
      <div class="stat-val stat-num" style="font-size:10px">${Math.round((d.freeHeap||0)/1024)}K</div></div>`;

    // سنسورها در داشبورد
    try{
      const sensors=await api('/api/sensors');
      const activeSensors=sensors.filter(s=>s.hasValue);
      if(activeSensors.length>0){
        document.getElementById('sensorDashCard').style.display='block';
        const profMap={0:'%',1:'°C',2:'%RH',3:'cm',4:'V',5:''};
        const iconMap={0:'💧',1:'🌡️',2:'💦',3:'🪣',4:'⚡',5:'📡'};
        let sh='';
        for(const s of activeSensors){
          const isOld=s.ageS>300;
          const valStr=s.valueType===5?(s.lastValue>0.5?'باز':'بسته')
            :(s.lastValue.toFixed(s.valueType===1||s.valueType===4?1:0));
          const ageStr=s.ageS<60?s.ageS+'s':Math.floor(s.ageS/60)+'m';
          sh+=`<div class="sensor-card">
            <div class="sensor-icon-wrap ${isOld?'offline':'online'}">${iconMap[s.valueType]||'📡'}</div>
            <div style="flex:1">
              <div class="sensor-name-lbl">${s.name}</div>
              <div class="sensor-age">${isOld?'⚠️ آفلاین':'آخرین: '+ageStr+' پیش'}</div>
            </div>
            <div style="text-align:left">
              <div class="sensor-val">${valStr}</div>
              <div class="sensor-unit">${profMap[s.valueType]||''}</div>
            </div>
          </div>`;
        }
        document.getElementById('sensorDash').innerHTML=sh;
      }else{
        document.getElementById('sensorDashCard').style.display='none';
      }
    }catch(e){}

    if(d.sceneCount>0){
      document.getElementById('scenesQuick').style.display='block';
      const scenes=await api('/api/scenes');
      let sqHtml='<div style="display:flex;flex-wrap:wrap;gap:6px">';
      for(const sc of scenes)
        sqHtml+=`<button class="btn btn-ghost btn-sm" onclick="runScene(${sc.id})">▶️ ${sc.name}</button>`;
      sqHtml+='</div>';
      document.getElementById('sceneQuickList').innerHTML=sqHtml;
    }
  }catch(e){}
}

async function toggleRelayUI(i){
  const relays=(await api('/api/status')).relays;
  const mode=relays[i].active?'off':'on';
  await api('/api/relay','POST',{index:i,mode});
  doRefresh();
}
async function allOff(){
  for(let i=0;i<4;i++)await api('/api/relay','POST',{index:i,mode:'off'});
  toast('همه رله‌ها خاموش شدند');doRefresh();
}
async function allOn(){
  for(let i=0;i<4;i++)await api('/api/relay','POST',{index:i,mode:'on'});
  toast('همه رله‌ها روشن شدند');doRefresh();
}

// ══════════════════════════════════════════════
//  SENSOR PAGE - SIMPLE MODE
// ══════════════════════════════════════════════
function setSensorMode(mode){
  S.sensorMode=mode;
  document.getElementById('sModeSim').classList.toggle('active',mode==='simple');
  document.getElementById('sModeAdv').classList.toggle('active',mode==='adv');
  document.getElementById('sSimpleMode').style.display=mode==='simple'?'block':'none';
  document.getElementById('sAdvMode').style.display=mode==='adv'?'block':'none';
}

function buildProfileGrid(){
  let h='';
  for(const p of SENSOR_PROFILES){
    h+=`<div class="profile-btn" id="prof_${p.id}" onclick="selectProfile('${p.id}')">
      <span class="pb-icon">${p.icon}</span>
      <div class="pb-name">${p.name}</div>
      <div class="pb-desc">${p.desc}</div>
    </div>`;
  }
  document.getElementById('profileGrid').innerHTML=h;
}

function selectProfile(id){
  S.selectedProfile=SENSOR_PROFILES.find(p=>p.id===id);
  if(!S.selectedProfile)return;
  document.querySelectorAll('.profile-btn').forEach(b=>b.classList.remove('selected'));
  document.getElementById('prof_'+id)?.classList.add('selected');

  document.getElementById('sStep1').style.display='none';
  document.getElementById('sStep2').style.display='block';
  document.getElementById('sStep2Icon').textContent=S.selectedProfile.icon;
  document.getElementById('sStep2Title').textContent=S.selectedProfile.name;
  document.getElementById('sStep2Desc').textContent=S.selectedProfile.hint;
  document.getElementById('sSaveSimpleBtn').disabled=true;
  S.simpleLearnData=null;

  // reset learn state
  document.getElementById('sSimpleLearnBox').style.display='block';
  document.getElementById('sSimpleLearnActive').style.display='none';
  document.getElementById('sSimpleLearnDone').style.display='none';
}

function sBackToStep1(){
  clearInterval(S.simpleLearnTimer);
  api('/api/sensors/learn/cancel','POST').catch(()=>{});
  document.getElementById('sStep1').style.display='block';
  document.getElementById('sStep2').style.display='none';
  S.selectedProfile=null;
  S.simpleLearnData=null;
}

function sSimpleLearn(){
  S.simpleLearnCD=30;
  document.getElementById('sSimpleLearnBox').style.display='none';
  document.getElementById('sSimpleLearnActive').style.display='block';
  document.getElementById('sSimpleLearnDone').style.display='none';
  document.getElementById('sSimpleCD').textContent=30;
  api('/api/sensors/learn/start','POST',{}).then(()=>{
    S.simpleLearnTimer=setInterval(async()=>{
      S.simpleLearnCD--;
      document.getElementById('sSimpleCD').textContent=S.simpleLearnCD;
      if(S.simpleLearnCD<=0){
        clearInterval(S.simpleLearnTimer);
        sSimpleCancel();
        toast('زمان یادگیری تمام شد','warning');
        return;
      }
      try{
        const d=await api('/api/sensors/learn/status');
        if(d.done){
          clearInterval(S.simpleLearnTimer);
          S.simpleLearnData=d;
          document.getElementById('sSimpleLearnActive').style.display='none';
          document.getElementById('sSimpleLearnDone').style.display='block';
          document.getElementById('sSimpleCode').textContent=
            '0x'+(d.rawCode>>>0).toString(16).toUpperCase().padStart(8,'0');
          document.getElementById('sSaveSimpleBtn').disabled=false;
          toast('سنسور شناسایی شد ✓','success');
        }
      }catch(e){}
    },1000);
  });
}

function sSimpleCancel(){
  clearInterval(S.simpleLearnTimer);
  api('/api/sensors/learn/cancel','POST').catch(()=>{});
  document.getElementById('sSimpleLearnBox').style.display='block';
  document.getElementById('sSimpleLearnActive').style.display='none';
  document.getElementById('sSimpleLearnDone').style.display='none';
  document.getElementById('sSaveSimpleBtn').disabled=true;
  S.simpleLearnData=null;
}

async function sSaveSimple(){
  const p=S.selectedProfile;
  const d=S.simpleLearnData;
  if(!p||!d){toast('ابتدا سنسور را یاد بدهید','warning');return;}
  const name=document.getElementById('sSimpleName').value.trim()||(p.name+' جدید');
  const payload={
    id:-1, name, valueType:p.valueType,
    baseCode:d.baseCode||0, baseMask:d.baseMask||p.baseMask,
    valueBits:p.valueBits, scale:p.scale, offset:p.offset,
    protocol:d.protocol||1, bitLength:d.bitLength||24
  };
  await api('/api/sensors/save','POST',payload);
  toast('سنسور ذخیره شد ✓');
  sBackToStep1();
  loadSensorsPage();
}

// ══════════════════════════════════════════════
//  SENSOR PAGE - ADVANCED MODE
// ══════════════════════════════════════════════
const S_sLearn={timer:null,cd:30};
function sLearnStart(){
  S_sLearn.cd=30;
  api('/api/sensors/learn/start','POST',{}).then(()=>{
    document.getElementById('sLearnIdle').style.display='none';
    document.getElementById('sLearnBox').style.display='block';
    document.getElementById('sLearnCfg').style.display='none';
    document.getElementById('sLearnCD').textContent=30;
    S_sLearn.timer=setInterval(async()=>{
      S_sLearn.cd--;
      document.getElementById('sLearnCD').textContent=S_sLearn.cd;
      if(S_sLearn.cd<=0){clearInterval(S_sLearn.timer);sLearnReset();toast('زمان تمام شد','warning');return;}
      try{
        const d=await api('/api/sensors/learn/status');
        if(d.done){
          clearInterval(S_sLearn.timer);
          document.getElementById('sLearnBox').style.display='none';
          document.getElementById('sLearnCfg').style.display='block';
          document.getElementById('sLearnInfo').textContent=
            'کد: 0x'+(d.rawCode>>>0).toString(16).toUpperCase().padStart(8,'0')+
            ' | Base: 0x'+(d.baseCode>>>0).toString(16).toUpperCase().padStart(8,'0');
          window._sLearnData=d;
          toast('کد دریافت شد ✓');
        }
      }catch(e){}
    },1000);
  });
}
function sLearnCancel(){sLearnReset();}
function sLearnReset(){
  clearInterval(S_sLearn.timer);
  api('/api/sensors/learn/cancel','POST').catch(()=>{});
  document.getElementById('sLearnIdle').style.display='block';
  document.getElementById('sLearnBox').style.display='none';
  document.getElementById('sLearnCfg').style.display='none';
  window._sLearnData=null;
}
async function sLearnSave(){
  const d=window._sLearnData||{};
  const payload={
    id:-1,
    name:document.getElementById('sNewName').value.trim()||'سنسور',
    valueType:parseInt(document.getElementById('sNewType').value),
    baseCode:d.baseCode||0, baseMask:d.baseMask||0xFFFFFF,
    valueBits:parseInt(document.getElementById('sNewValueBits').value)||8,
    scale:parseFloat(document.getElementById('sNewScale').value)||1.0,
    offset:parseFloat(document.getElementById('sNewOffset').value)||0,
    protocol:d.protocol||1, bitLength:d.bitLength||24
  };
  await api('/api/sensors/save','POST',payload);
  toast('سنسور ذخیره شد ✓');
  sLearnReset();
  loadSensorsPage();
}

// ── لیست سنسورها ─────────────────────────────────────────────────
async function loadSensorsPage(){
  const sensors=await api('/api/sensors');
  document.getElementById('sensorCounter').textContent=sensors.length+' سنسور';

  const typeLabels=['درصد','دما','رطوبت','فاصله','ولتاژ','خام'];
  const icons={0:'💧',1:'🌡️',2:'💦',3:'🪣',4:'⚡',5:'📡'};
  const units={0:'%',1:'°C',2:'%RH',3:'cm',4:'V',5:''};

  if(!sensors.length){
    const emptyHtml='<div class="empty"><span class="empty-icon">🌡️</span><p>هنوز سنسوری ثبت نشده</p></div>';
    document.getElementById('sensorList').innerHTML=emptyHtml;
    document.getElementById('sensorListAdv').innerHTML=emptyHtml;
    return;
  }

  // حالت ساده
  let hSimple='';
  for(const s of sensors){
    const ageOk=s.hasValue&&s.ageS<300;
    const isDoor=s.valueType===5;
    const valStr=isDoor?(s.lastValue>0.5?'🔓 باز':'🔒 بسته')
      :(s.hasValue?(s.lastValue.toFixed(1)+' '+(units[s.valueType]||'')):'— دریافت نشده');
    const ageStr=s.hasValue?(s.ageS<60?s.ageS+'s':Math.floor(s.ageS/60)+'m پیش'):'هرگز';
    hSimple+=`<div class="sensor-card">
      <div class="sensor-icon-wrap ${ageOk?'online':'offline'}">${icons[s.valueType]||'📡'}</div>
      <div style="flex:1">
        <div class="sensor-name-lbl">${s.name}</div>
        <div class="sensor-age">${ageOk?('آخرین: '+ageStr):(!s.hasValue?'هنوز دریافت نشده':'⚠️ آفلاین')}</div>
      </div>
      <div style="text-align:left;display:flex;flex-direction:column;align-items:flex-end;gap:6px">
        <div style="font-size:16px;font-weight:700;color:var(--text)">${valStr}</div>
        <div style="display:flex;gap:4px">
          <button class="btn btn-ghost btn-sm" onclick="sOpenEdit(${s.id})">⚙️</button>
          <button class="btn btn-danger btn-sm" onclick="sDelete(${s.id})">🗑️</button>
        </div>
      </div>
    </div>`;
  }
  document.getElementById('sensorList').innerHTML=hSimple;

  // حالت پیشرفته
  let hAdv='';
  for(const s of sensors){
    const ageOk=s.hasValue&&s.ageS<300;
    const valStr=s.hasValue?s.lastValue.toFixed(1)+(units[s.valueType]||''):'—';
    hAdv+=`<div class="list-item">
      <div class="item-icon" style="background:${ageOk?'var(--success)':'var(--border)'}">
        ${icons[s.valueType]||'📡'}
      </div>
      <div class="item-body">
        <div class="item-title">${s.name} <span class="tag tag-relay">${typeLabels[s.valueType]||'خام'}</span></div>
        <div class="item-sub">
          مقدار: <b>${valStr}</b>
          ${s.hasValue?' | '+(s.ageS<60?s.ageS+'s':Math.floor(s.ageS/60)+'m')+' پیش':' | هنوز دریافت نشده'}
          <br>base: 0x${(s.baseCode>>>0).toString(16).toUpperCase().padStart(6,'0')}
          | ${s.valueBits}bit × ${s.scale}
        </div>
      </div>
      <div class="item-actions">
        <button class="btn btn-ghost btn-sm" onclick="sOpenEdit(${s.id})">⚙️</button>
        <button class="btn btn-danger btn-sm" onclick="sDelete(${s.id})">🗑️</button>
      </div>
    </div>`;
  }
  document.getElementById('sensorListAdv').innerHTML=hAdv;
}

function sOpenEdit(id){
  api('/api/sensors').then(list=>{
    const s=list.find(x=>x.id===id);
    if(!s)return;
    document.getElementById('sEditId').value=id;
    document.getElementById('sEditName').value=s.name;
    document.getElementById('sEditType').value=s.valueType;
    document.getElementById('sEditValueBits').value=s.valueBits;
    document.getElementById('sEditScale').value=s.scale;
    document.getElementById('sEditOffset').value=s.offset;
    openModal('sEditModal');
  });
}
async function sEditSave(){
  const id=parseInt(document.getElementById('sEditId').value);
  const sensors=await api('/api/sensors');
  const s=sensors.find(x=>x.id===id);
  if(!s)return;
  const payload=Object.assign({},s,{
    id,
    name:document.getElementById('sEditName').value.trim()||s.name,
    valueType:parseInt(document.getElementById('sEditType').value),
    valueBits:parseInt(document.getElementById('sEditValueBits').value),
    scale:parseFloat(document.getElementById('sEditScale').value),
    offset:parseFloat(document.getElementById('sEditOffset').value)
  });
  await api('/api/sensors/save','POST',payload);
  toast('ذخیره شد ✓');
  closeModal('sEditModal');
  loadSensorsPage();
}
async function sDelete(id){
  showConfirm('این سنسور حذف شود؟','حذف',async()=>{
    await api('/api/sensors/delete','POST',{id});
    toast('سنسور حذف شد','info');
    loadSensorsPage();
  });
}

// ══════════════════════════════════════════════
//  AUTOMATION - TEMPLATE WIZARD
// ══════════════════════════════════════════════
function setAutoMode(mode){
  S.autoMode=mode;
  document.getElementById('aModeSim').classList.toggle('active',mode==='simple');
  document.getElementById('aModeAdv').classList.toggle('active',mode==='adv');
  document.getElementById('aSimpleMode').style.display=mode==='simple'?'block':'none';
  document.getElementById('aAdvMode').style.display=mode==='adv'?'block':'none';
}

async function openTemplate(type){
  S.currentTemplate=type;
  const sensors=await api('/api/sensors');
  const relayOpts='<option value="0">رله ۱</option><option value="1">رله ۲</option><option value="2">رله ۳</option><option value="3">رله ۴</option>';
  const tempSensors=sensors.filter(s=>s.valueType===1);
  const soilSensors=sensors.filter(s=>s.valueType===0);
  const levelSensors=sensors.filter(s=>s.valueType===3);
  const doorSensors=sensors.filter(s=>s.valueType===5);

  const sOpts=(arr,label)=>{
    if(!arr.length)return `<option value="-1">— ${label} ثبت نشده —</option>`;
    return arr.map(s=>`<option value="${s.id}">${s.name}</option>`).join('');
  };

  const templates={
    irrigation:{
      title:'🌱 آبیاری کشاورزی',
      html:`
      <div class="wizard-step">
        <div class="wizard-num">۱</div>
        <div class="wizard-q">کدام رله پمپ آب است؟</div>
        <select id="tmpl_relay">${relayOpts}</select>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۲</div>
        <div class="wizard-q">آبیاری چه ساعتی شروع شود؟</div>
        <div class="step-grid">
          <div><label class="form-label">ساعت:</label>
            <input type="number" id="tmpl_hour" min="0" max="23" value="6"></div>
          <div><label class="form-label">دقیقه:</label>
            <input type="number" id="tmpl_min" min="0" max="59" value="0"></div>
        </div>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۳</div>
        <div class="wizard-q">چند دقیقه آبیاری شود؟</div>
        <input type="number" id="tmpl_duration" value="30" min="1" max="480">
        <div style="font-size:10px;color:var(--text2);margin-top:4px">دقیقه</div>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۴</div>
        <div class="wizard-q">روزهای آبیاری:</div>
        <div style="display:flex;flex-wrap:wrap;gap:6px;margin-top:6px">
          ${['ی','د','س','چ','پ','ج','ش'].map((d,i)=>
            `<label style="display:flex;align-items:center;gap:4px;font-size:12px;cursor:pointer">
              <input type="checkbox" class="tmpl_wday" value="${i}" checked
                style="width:16px;height:16px;accent-color:var(--primary)">${d}</label>`
          ).join('')}
        </div>
      </div>`
    },
    thermostat:{
      title:'🌡️ کنترل دما',
      html:`
      <div class="wizard-step">
        <div class="wizard-num">۱</div>
        <div class="wizard-q">کدام سنسور دما؟</div>
        <select id="tmpl_sensor">${sOpts(tempSensors,'سنسور دما')}</select>
        ${!tempSensors.length?'<p style="font-size:10px;color:var(--warning);margin-top:6px">⚠️ ابتدا یک سنسور دما ثبت کنید</p>':''}
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۲</div>
        <div class="wizard-q">کدام رله خنک‌کننده/گرمازا است؟</div>
        <select id="tmpl_relay">${relayOpts}</select>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۳</div>
        <div class="wizard-q">دمای روشن شدن (°C):</div>
        <input type="number" id="tmpl_onTemp" value="30" step="0.5">
        <div style="font-size:10px;color:var(--text2);margin-top:4px">بالاتر از این دما → روشن</div>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۴</div>
        <div class="wizard-q">دمای خاموش شدن (°C):</div>
        <input type="number" id="tmpl_offTemp" value="27" step="0.5">
        <div style="font-size:10px;color:var(--text2);margin-top:4px">پایین‌تر از این دما → خاموش</div>
      </div>`
    },
    soilmoisture:{
      title:'💧 رطوبت خاک',
      html:`
      <div class="wizard-step">
        <div class="wizard-num">۱</div>
        <div class="wizard-q">کدام سنسور رطوبت خاک؟</div>
        <select id="tmpl_sensor">${sOpts(soilSensors,'سنسور رطوبت خاک')}</select>
        ${!soilSensors.length?'<p style="font-size:10px;color:var(--warning);margin-top:6px">⚠️ ابتدا یک سنسور رطوبت خاک ثبت کنید</p>':''}
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۲</div>
        <div class="wizard-q">کدام رله پمپ آب است؟</div>
        <select id="tmpl_relay">${relayOpts}</select>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۳</div>
        <div class="wizard-q">آبیاری وقتی رطوبت زیر چند درصد باشد:</div>
        <input type="number" id="tmpl_dryThresh" value="30" min="0" max="100">
        <div style="font-size:10px;color:var(--text2);margin-top:4px">پمپ روشن می‌شود</div>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۴</div>
        <div class="wizard-q">آبیاری متوقف شود وقتی رطوبت به چند درصد رسید:</div>
        <input type="number" id="tmpl_wetThresh" value="70" min="0" max="100">
        <div style="font-size:10px;color:var(--text2);margin-top:4px">پمپ خاموش می‌شود</div>
      </div>`
    },
    level:{
      title:'🪣 سطح مایع',
      html:`
      <div class="wizard-step">
        <div class="wizard-num">۱</div>
        <div class="wizard-q">کدام سنسور سطح مایع؟</div>
        <select id="tmpl_sensor">${sOpts(levelSensors,'سنسور سطح')}</select>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۲</div>
        <div class="wizard-q">کدام رله؟</div>
        <select id="tmpl_relay">${relayOpts}</select>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۳</div>
        <div class="wizard-q">چه اتفاقی بیفتد؟</div>
        <select id="tmpl_levelAction">
          <option value="low_on">وقتی تانک خالی شد → پمپ ورودی روشن شود</option>
          <option value="high_off">وقتی تانک پر شد → پمپ خاموش شود</option>
          <option value="low_sms">وقتی تانک خالی شد → SMS هشدار</option>
        </select>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۴</div>
        <div class="wizard-q">آستانه سطح (cm):</div>
        <input type="number" id="tmpl_levelThresh" value="10" min="0">
      </div>`
    },
    door:{
      title:'🚪 در و پنجره',
      html:`
      <div class="wizard-step">
        <div class="wizard-num">۱</div>
        <div class="wizard-q">کدام سنسور در/پنجره؟</div>
        <select id="tmpl_sensor">${sOpts(doorSensors,'سنسور در')}</select>
        ${!doorSensors.length?'<p style="font-size:10px;color:var(--warning);margin-top:6px">⚠️ ابتدا یک سنسور در/پنجره ثبت کنید</p>':''}
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۲</div>
        <div class="wizard-q">وقتی در باز شد چه شود؟</div>
        <select id="tmpl_doorAction">
          <option value="sms">SMS هشدار ارسال شود</option>
          <option value="relay_on">یک رله روشن شود (مثل چراغ)</option>
          <option value="both">هم SMS هم روشن شود</option>
        </select>
      </div>
      <div class="wizard-step" id="tmpl_doorRelayStep">
        <div class="wizard-num">۳</div>
        <div class="wizard-q">کدام رله؟</div>
        <select id="tmpl_relay">${relayOpts}</select>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۴</div>
        <div class="wizard-q">متن SMS هشدار:</div>
        <input type="text" id="tmpl_smsText" value="هشدار: در باز شد!">
      </div>`
    },
    timedrelay:{
      title:'⏰ زمان‌بندی ساده',
      html:`
      <div class="wizard-step">
        <div class="wizard-num">۱</div>
        <div class="wizard-q">کدام رله؟</div>
        <select id="tmpl_relay">${relayOpts}</select>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۲</div>
        <div class="wizard-q">ساعت روشن شدن:</div>
        <div class="step-grid">
          <div><label class="form-label">ساعت:</label>
            <input type="number" id="tmpl_onHour" min="0" max="23" value="7"></div>
          <div><label class="form-label">دقیقه:</label>
            <input type="number" id="tmpl_onMin" min="0" max="59" value="0"></div>
        </div>
      </div>
      <div class="wizard-step">
        <div class="wizard-num">۳</div>
        <div class="wizard-q">ساعت خاموش شدن:</div>
        <div class="step-grid">
          <div><label class="form-label">ساعت:</label>
            <input type="number" id="tmpl_offHour" min="0" max="23" value="22"></div>
          <div><label class="form-label">دقیقه:</label>
            <input type="number" id="tmpl_offMin" min="0" max="59" value="0"></div>
        </div>
      </div>`
    }
  };

  const t=templates[type];
  if(!t)return;
  document.getElementById('tmplTitle').textContent=t.title;
  document.getElementById('tmplBody').innerHTML=t.html;
  openModal('templateModal');
}

async function saveTemplate(){
  const type=S.currentTemplate;
  if(!type)return;

  let automationPayload=null;
  const relay=parseInt(document.getElementById('tmpl_relay')?.value||0);

  if(type==='irrigation'){
    const hour=parseInt(document.getElementById('tmpl_hour').value||6);
    const min=parseInt(document.getElementById('tmpl_min').value||0);
    const dur=parseInt(document.getElementById('tmpl_duration').value||30)*60*1000;
    let mask=0;
    document.querySelectorAll('.tmpl_wday').forEach(cb=>{if(cb.checked)mask|=(1<<parseInt(cb.value));});
    if(mask===0)mask=0x7F;
    automationPayload={
      id:-1, name:'آبیاری خودکار', active:true, logicOp:0,
      conditions:[{type:3,hourStart:hour,minuteStart:min,hourEnd:hour,minuteEnd:min,weekdayMask:mask,negate:false,relayId:0,sensorId:0,thresh1:0,thresh2:100,offlineMinutes:10}],
      actions:[{type:4,targetId:relay,durationMs:dur,smsText:'',pulseOnMs:500,pulseOffMs:500,pulseCount:3,delayBeforeMs:0}],
      hysteresis:{enabled:false,onThreshold:0,offThreshold:0,sensorId:0,relayId:0},
      cooldownEnabled:true,cooldownMinutes:60
    };
  } else if(type==='thermostat'){
    const sensor=parseInt(document.getElementById('tmpl_sensor')?.value||-1);
    if(sensor<0){toast('سنسور دما ثبت نشده','error');return;}
    const onT=parseFloat(document.getElementById('tmpl_onTemp').value||30);
    const offT=parseFloat(document.getElementById('tmpl_offTemp').value||27);
    automationPayload={
      id:-1, name:'ترموستات', active:true, logicOp:0,
      conditions:[{type:4,sensorId:sensor,thresh1:onT,thresh2:100,negate:false,relayId:0,hourStart:0,hourEnd:23,minuteStart:0,minuteEnd:59,weekdayMask:0x7F,offlineMinutes:10}],
      actions:[{type:1,targetId:relay,durationMs:0,smsText:'',pulseOnMs:500,pulseOffMs:500,pulseCount:3,delayBeforeMs:0}],
      hysteresis:{enabled:true,onThreshold:onT,offThreshold:offT,sensorId:sensor,relayId:relay},
      cooldownEnabled:false,cooldownMinutes:5
    };
  } else if(type==='soilmoisture'){
    const sensor=parseInt(document.getElementById('tmpl_sensor')?.value||-1);
    if(sensor<0){toast('سنسور رطوبت خاک ثبت نشده','error');return;}
    const dry=parseFloat(document.getElementById('tmpl_dryThresh').value||30);
    const wet=parseFloat(document.getElementById('tmpl_wetThresh').value||70);
    automationPayload={
      id:-1, name:'آبیاری رطوبت خاک', active:true, logicOp:0,
      conditions:[{type:5,sensorId:sensor,thresh1:dry,thresh2:100,negate:false,relayId:0,hourStart:0,hourEnd:23,minuteStart:0,minuteEnd:59,weekdayMask:0x7F,offlineMinutes:10}],
      actions:[{type:1,targetId:relay,durationMs:0,smsText:'',pulseOnMs:500,pulseOffMs:500,pulseCount:3,delayBeforeMs:0}],
      hysteresis:{enabled:true,onThreshold:dry,offThreshold:wet,sensorId:sensor,relayId:relay},
      cooldownEnabled:true,cooldownMinutes:30
    };
  } else if(type==='level'){
    const sensor=parseInt(document.getElementById('tmpl_sensor')?.value||-1);
    const action=document.getElementById('tmpl_levelAction')?.value||'low_on';
    const thresh=parseFloat(document.getElementById('tmpl_levelThresh').value||10);
    const acts=[];
    if(action==='low_sms'){
      acts.push({type:7,targetId:0,durationMs:0,smsText:'هشدار: سطح مایع پایین است!',pulseOnMs:500,pulseOffMs:500,pulseCount:3,delayBeforeMs:0});
    } else {
      acts.push({type:action==='low_on'?1:2,targetId:relay,durationMs:0,smsText:'',pulseOnMs:500,pulseOffMs:500,pulseCount:3,delayBeforeMs:0});
    }
    automationPayload={
      id:-1, name:'کنترل سطح مایع', active:true, logicOp:0,
      conditions:[{type:action==='high_off'?4:5,sensorId:Math.max(0,sensor),thresh1:thresh,thresh2:200,negate:false,relayId:0,hourStart:0,hourEnd:23,minuteStart:0,minuteEnd:59,weekdayMask:0x7F,offlineMinutes:10}],
      actions:acts,
      hysteresis:{enabled:false,onThreshold:0,offThreshold:0,sensorId:0,relayId:0},
      cooldownEnabled:true,cooldownMinutes:10
    };
  } else if(type==='door'){
    const sensor=parseInt(document.getElementById('tmpl_sensor')?.value||-1);
    if(sensor<0){toast('سنسور در ثبت نشده','error');return;}
    const action=document.getElementById('tmpl_doorAction')?.value||'sms';
    const smsText=document.getElementById('tmpl_smsText')?.value||'هشدار: در باز شد!';
    const acts=[];
    if(action==='sms'||action==='both')
      acts.push({type:7,targetId:0,durationMs:0,smsText,pulseOnMs:500,pulseOffMs:500,pulseCount:3,delayBeforeMs:0});
    if(action==='relay_on'||action==='both')
      acts.push({type:1,targetId:relay,durationMs:0,smsText:'',pulseOnMs:500,pulseOffMs:500,pulseCount:3,delayBeforeMs:0});
    automationPayload={
      id:-1, name:'هشدار در/پنجره', active:true, logicOp:0,
      conditions:[{type:4,sensorId:sensor,thresh1:0.5,thresh2:2,negate:false,relayId:0,hourStart:0,hourEnd:23,minuteStart:0,minuteEnd:59,weekdayMask:0x7F,offlineMinutes:10}],
      actions:acts,
      hysteresis:{enabled:false,onThreshold:0,offThreshold:0,sensorId:0,relayId:0},
      cooldownEnabled:true,cooldownMinutes:5
    };
  } else if(type==='timedrelay'){
    const onH=parseInt(document.getElementById('tmpl_onHour').value||7);
    const onM=parseInt(document.getElementById('tmpl_onMin').value||0);
    const offH=parseInt(document.getElementById('tmpl_offHour').value||22);
    const offM=parseInt(document.getElementById('tmpl_offMin').value||0);
    // دو اتوماسیون: یکی روشن یکی خاموش
    const payOn={
      id:-1, name:`رله ${relay+1} روشن ${onH}:${String(onM).padStart(2,'0')}`,
      active:true, logicOp:0,
      conditions:[{type:3,hourStart:onH,minuteStart:onM,hourEnd:onH,minuteEnd:onM,weekdayMask:0x7F,negate:false,relayId:0,sensorId:0,thresh1:0,thresh2:100,offlineMinutes:10}],
      actions:[{type:1,targetId:relay,durationMs:0,smsText:'',pulseOnMs:500,pulseOffMs:500,pulseCount:3,delayBeforeMs:0}],
      hysteresis:{enabled:false,onThreshold:0,offThreshold:0,sensorId:0,relayId:0},
      cooldownEnabled:true,cooldownMinutes:60
    };
    const payOff={
      id:-1, name:`رله ${relay+1} خاموش ${offH}:${String(offM).padStart(2,'0')}`,
      active:true, logicOp:0,
      conditions:[{type:3,hourStart:offH,minuteStart:offM,hourEnd:offH,minuteEnd:offM,weekdayMask:0x7F,negate:false,relayId:0,sensorId:0,thresh1:0,thresh2:100,offlineMinutes:10}],
      actions:[{type:2,targetId:relay,durationMs:0,smsText:'',pulseOnMs:500,pulseOffMs:500,pulseCount:3,delayBeforeMs:0}],
      hysteresis:{enabled:false,onThreshold:0,offThreshold:0,sensorId:0,relayId:0},
      cooldownEnabled:true,cooldownMinutes:60
    };
    await api('/api/automations/save','POST',payOn);
    await api('/api/automations/save','POST',payOff);
    toast('زمان‌بندی ذخیره شد ✓');
    closeModal('templateModal');
    loadAutomations();
    return;
  }

  if(automationPayload){
    await api('/api/automations/save','POST',automationPayload);
    toast('اتوماسیون ذخیره شد ✓');
    closeModal('templateModal');
    loadAutomations();
  }
}

// ══════════════════════════════════════════════
//  AUTOMATION - ADVANCED
// ══════════════════════════════════════════════
let autoCondCount=0, autoActCount=0, autoSensors=[], autoScenes=[];

async function loadAutomations(){
  const list=await api('/api/automations');
  document.getElementById('autoCounter').textContent=list.length+' اتوماسیون';

  // لیست ساده
  if(!list.length){
    document.getElementById('autoSimpleList').innerHTML=
      '<div class="empty"><span class="empty-icon">🤖</span><p>اتوماسیونی تعریف نشده</p></div>';
    document.getElementById('autoList').innerHTML=
      '<div class="empty"><span class="empty-icon">🤖</span><p>اتوماسیونی تعریف نشده</p></div>';
    return;
  }

  let h='';
  for(const a of list){
    const condSummary=a.conditions.map(c=>
      `<span class="tag tag-single">${c.negate?'NOT ':''} ${c.typeName}</span>`
    ).join(`<span style="color:var(--warning);font-size:9px;margin:0 2px">${a.logicOp===0?'AND':'OR'}</span>`);
    const actSummary=a.actions.map(ac=>
      `<span class="tag tag-relay">${ac.typeName}</span>`).join('');
    h+=`<div class="list-item" style="align-items:flex-start;flex-direction:column;gap:8px">
      <div style="display:flex;width:100%;align-items:center;gap:8px">
        <div class="item-icon" style="background:${a.active?'linear-gradient(135deg,var(--primary),#a855f7)':'var(--border)'}">🤖</div>
        <div class="item-body">
          <div class="item-title">${a.name}
            ${a.hysteresis&&a.hysteresis.enabled?'<span class="tag tag-double">🌡️ Hyst</span>':''}
            ${a.cooldownEnabled?`<span class="tag tag-long">⏳ ${a.cooldownMinutes}m</span>`:''}
          </div>
        </div>
        <div style="display:flex;gap:6px;flex-shrink:0">
          <button class="btn btn-ghost btn-sm" onclick="editAuto(${a.id})">✏️</button>
          <button class="btn btn-warning btn-sm" onclick="testAuto(${a.id})">▶️</button>
          <button class="btn btn-${a.active?'success':'ghost'} btn-sm" onclick="toggleAuto(${a.id})">${a.active?'✅':'⏸️'}</button>
          <button class="btn btn-danger btn-sm" onclick="deleteAuto(${a.id})">🗑️</button>
        </div>
      </div>
      <div style="padding-right:44px;width:100%">
        <div style="font-size:10px;color:var(--text2);margin-bottom:4px">IF:</div>
        <div>${condSummary||'—'}</div>
        <div style="font-size:10px;color:var(--text2);margin:6px 0 4px">THEN:</div>
        <div>${actSummary||'—'}</div>
      </div>
    </div>`;
  }
  document.getElementById('autoSimpleList').innerHTML=h;
  document.getElementById('autoList').innerHTML=h;
}

async function openAutoModal(existingData=null){
  try{autoSensors=await api('/api/sensors');autoScenes=await api('/api/scenes');}
  catch(e){autoSensors=[];autoScenes=[];}
  autoCondCount=0;autoActCount=0;
  document.getElementById('autoCondList').innerHTML='';
  document.getElementById('autoActList').innerHTML='';
  document.getElementById('autoName').value='';
  document.getElementById('autoLogicOp').value='0';
  document.getElementById('autoEditId').value='-1';
  document.getElementById('autoHystEnabled').checked=false;
  document.getElementById('autoHystFields').style.display='none';
  document.getElementById('autoCoolEnabled').checked=false;
  document.getElementById('autoCoolFields').style.display='none';
  document.getElementById('autoCoolMin').value='5';
  let sOpts='<option value="-1">— بدون سنسور —</option>';
  for(const s of autoSensors)sOpts+=`<option value="${s.id}">${s.name}</option>`;
  document.getElementById('autoHystSensor').innerHTML=sOpts;
  if(existingData){
    document.getElementById('autoEditId').value=existingData.id;
    document.getElementById('autoName').value=existingData.name;
    document.getElementById('autoLogicOp').value=existingData.logicOp;
    document.getElementById('autoCoolEnabled').checked=existingData.cooldownEnabled;
    document.getElementById('autoCoolFields').style.display=existingData.cooldownEnabled?'block':'none';
    document.getElementById('autoCoolMin').value=existingData.cooldownMinutes;
    if(existingData.hysteresis&&existingData.hysteresis.enabled){
      document.getElementById('autoHystEnabled').checked=true;
      document.getElementById('autoHystFields').style.display='block';
      document.getElementById('autoHystSensor').value=existingData.hysteresis.sensorId;
      document.getElementById('autoHystRelay').value=existingData.hysteresis.relayId;
      document.getElementById('autoHystOn').value=existingData.hysteresis.onThreshold;
      document.getElementById('autoHystOff').value=existingData.hysteresis.offThreshold;
    }
    for(const c of existingData.conditions)addAutoCond(c);
    for(const ac of existingData.actions)addAutoAct(ac);
  } else {
    addAutoCond();addAutoAct();
  }
  openModal('autoModal');
}

function addAutoCond(data=null){
  const id=autoCondCount++;
  const sOpts=autoSensors.map(s=>`<option value="${s.id}">${s.name}</option>`).join('');
  const h=`<div class="step-card" id="autoCond${id}">
    <div class="step-num">${id+1}</div>
    <div class="form-group" style="margin-top:8px">
      <label class="form-label">نوع شرط:</label>
      <select id="aCondType${id}" onchange="updateCondUI(${id})">
        <option value="0">همیشه</option>
        <option value="1">رله روشن باشد</option>
        <option value="2">رله خاموش باشد</option>
        <option value="3">بین ساعت</option>
        <option value="4">سنسور بزرگ‌تر از</option>
        <option value="5">سنسور کوچک‌تر از</option>
        <option value="6">سنسور بین دو مقدار</option>
        <option value="7">سنسور خارج از بازه</option>
        <option value="8">سنسور آفلاین</option>
        <option value="9">روز هفته</option>
      </select>
    </div>
    <div id="aCondRelay${id}" style="display:none">
      <select id="aCondRelayId${id}"><option value="0">رله ۱</option><option value="1">رله ۲</option><option value="2">رله ۳</option><option value="3">رله ۴</option></select>
    </div>
    <div id="aCondSensor${id}" style="display:none">
      <select id="aCondSensorId${id}">${sOpts||'<option>—</option>'}</select>
    </div>
    <div id="aCondThresh${id}" style="display:none">
      <div class="step-grid" style="margin-top:6px">
        <div class="form-group" style="margin:0"><label class="form-label">مقدار ۱:</label>
          <input type="number" id="aCondT1_${id}" value="${data?data.thresh1:30}" step="0.1"></div>
        <div class="form-group" id="aCondT2G${id}" style="display:none;margin:0"><label class="form-label">مقدار ۲:</label>
          <input type="number" id="aCondT2_${id}" value="${data?data.thresh2:100}" step="0.1"></div>
      </div>
    </div>
    <div id="aCondTime${id}" style="display:none">
      <div class="step-grid" style="margin-top:6px">
        <div class="form-group" style="margin:0"><label class="form-label">از ساعت:</label>
          <input type="time" id="aCondTimeS${id}" value="${data?String(data.hourStart).padStart(2,'0')+':'+String(data.minuteStart).padStart(2,'0'):'08:00'}"></div>
        <div class="form-group" style="margin:0"><label class="form-label">تا ساعت:</label>
          <input type="time" id="aCondTimeE${id}" value="${data?String(data.hourEnd).padStart(2,'0')+':'+String(data.minuteEnd).padStart(2,'0'):'18:00'}"></div>
      </div>
    </div>
    <div id="aCondDay${id}" style="display:none">
      <div style="display:flex;flex-wrap:wrap;gap:6px;margin-top:6px">
        ${['ی','د','س','چ','پ','ج','ش'].map((d,i)=>
          `<label style="display:flex;align-items:center;gap:3px;font-size:12px;cursor:pointer">
            <input type="checkbox" class="aCondWday${id}" value="${i}"
              ${!data||(data.weekdayMask&(1<<i))?'checked':''}
              style="width:15px;height:15px;accent-color:var(--primary)">${d}</label>`
        ).join('')}
      </div>
    </div>
    <div id="aCondOffline${id}" style="display:none">
      <div class="form-group" style="margin-top:6px"><label class="form-label">آفلاین بیشتر از (دقیقه):</label>
        <input type="number" id="aCondOffMin${id}" value="${data?data.offlineMinutes:10}" min="1"></div>
    </div>
    <div class="checkbox-row" style="margin-top:8px">
      <input type="checkbox" id="aCondNot${id}" style="width:16px;height:16px" ${data&&data.negate?'checked':''}>
      <label for="aCondNot${id}" style="font-size:11px">NOT — معکوس این شرط</label>
    </div>
    ${id>0?`<button class="btn btn-danger btn-sm" onclick="document.getElementById('autoCond${id}').remove()" style="margin-top:8px;width:100%">🗑️ حذف شرط</button>`:''}
  </div>`;
  document.getElementById('autoCondList').insertAdjacentHTML('beforeend',h);
  if(data){
    document.getElementById('aCondType'+id).value=data.type;
    if(data.relayId!==undefined)document.getElementById('aCondRelayId'+id).value=data.relayId;
    if(data.sensorId!==undefined)document.getElementById('aCondSensorId'+id).value=data.sensorId;
  }
  updateCondUI(id);
}

function updateCondUI(id){
  const type=parseInt(document.getElementById('aCondType'+id).value);
  const show=(s,v)=>{const el=document.getElementById('aCond'+s+id);if(el)el.style.display=v?'block':'none';};
  show('Relay',type===1||type===2);
  show('Sensor',type>=4&&type<=8);
  show('Thresh',type>=4&&type<=7);
  show('Time',type===3);
  show('Day',type===9);
  show('Offline',type===8);
  const t2g=document.getElementById('aCondT2G'+id);
  if(t2g)t2g.style.display=(type===6||type===7)?'block':'none';
}

function addAutoAct(data=null){
  const id=autoActCount++;
  const sOpts=autoScenes.map(s=>`<option value="${s.id}">${s.name}</option>`).join('');
  const h=`<div class="step-card" id="autoAct${id}">
    <div class="step-num">${id+1}</div>
    <div class="form-group" style="margin-top:8px">
      <label class="form-label">نوع عمل:</label>
      <select id="aActType${id}" onchange="updateActUI(${id})">
        <option value="1">روشن کردن رله</option>
        <option value="2">خاموش کردن رله</option>
        <option value="3">Toggle رله</option>
        <option value="4">روشن زماندار</option>
        <option value="5">خاموش کردن همه</option>
        <option value="6">اجرای سناریو</option>
        <option value="7">ارسال SMS</option>
        <option value="8">Pulse رله</option>
      </select>
    </div>
    <div id="aActTarget${id}">
      <div class="form-group" style="margin:0"><label class="form-label">هدف:</label>
        <select id="aActTargetSel${id}">
          <option value="0">رله ۱</option><option value="1">رله ۲</option>
          <option value="2">رله ۳</option><option value="3">رله ۴</option>
        </select></div>
    </div>
    <div id="aActDur${id}" style="display:none">
      <div class="form-group" style="margin-top:6px"><label class="form-label">مدت (ثانیه):</label>
        <input type="number" id="aActDurVal${id}" value="${data?data.durationMs/1000:30}" min="1"></div>
    </div>
    <div id="aActSMS${id}" style="display:none">
      <div class="form-group" style="margin-top:6px"><label class="form-label">متن SMS:</label>
        <input type="text" id="aActSMSText${id}" value="${data?data.smsText:''}" placeholder="هشدار!"></div>
    </div>
    <div id="aActPulse${id}" style="display:none">
      <div class="step-grid" style="margin-top:6px">
        <div class="form-group" style="margin:0"><label class="form-label">ON (ms):</label>
          <input type="number" id="aActPON${id}" value="${data?data.pulseOnMs:500}" min="100"></div>
        <div class="form-group" style="margin:0"><label class="form-label">OFF (ms):</label>
          <input type="number" id="aActPOFF${id}" value="${data?data.pulseOffMs:500}" min="100"></div>
      </div>
      <div class="form-group" style="margin-top:6px"><label class="form-label">تعداد سیکل:</label>
        <input type="number" id="aActPCnt${id}" value="${data?data.pulseCount:3}" min="1" max="100"></div>
    </div>
    <div class="form-group" style="margin-top:6px"><label class="form-label">تأخیر قبل (ثانیه):</label>
      <input type="number" id="aActDelay${id}" value="${data?data.delayBeforeMs:0}" min="0"></div>
    ${id>0?`<button class="btn btn-danger btn-sm" onclick="document.getElementById('autoAct${id}').remove()" style="margin-top:8px;width:100%">🗑️ حذف عمل</button>`:''}
  </div>`;
  document.getElementById('autoActList').insertAdjacentHTML('beforeend',h);
  if(data)document.getElementById('aActType'+id).value=data.type;
  updateActUI(id);
}

function updateActUI(id){
  const type=parseInt(document.getElementById('aActType'+id).value);
  const targetSel=document.getElementById('aActTargetSel'+id);
  const showTarget=(type!==5&&type!==7);
  document.getElementById('aActTarget'+id).style.display=showTarget?'block':'none';
  if(showTarget){
    if(type===6){
      targetSel.innerHTML=autoScenes.map(s=>`<option value="${s.id}">${s.name}</option>`).join('')||'<option>—</option>';
    } else {
      targetSel.innerHTML='<option value="0">رله ۱</option><option value="1">رله ۲</option><option value="2">رله ۳</option><option value="3">رله ۴</option>';
    }
  }
  document.getElementById('aActDur'+id).style.display=type===4?'block':'none';
  document.getElementById('aActSMS'+id).style.display=type===7?'block':'none';
  document.getElementById('aActPulse'+id).style.display=type===8?'block':'none';
}

async function saveAutomation(){
  const id=parseInt(document.getElementById('autoEditId').value);
  const conditions=[];
  for(let i=0;i<autoCondCount;i++){
    if(!document.getElementById('autoCond'+i))continue;
    const type=parseInt(document.getElementById('aCondType'+i).value);
    let weekdayMask=0;
    document.querySelectorAll('.aCondWday'+i).forEach(cb=>{if(cb.checked)weekdayMask|=(1<<parseInt(cb.value));});
    if(weekdayMask===0)weekdayMask=0x7F;
    const timeS=(document.getElementById('aCondTimeS'+i)?.value||'08:00').split(':');
    const timeE=(document.getElementById('aCondTimeE'+i)?.value||'18:00').split(':');
    conditions.push({
      type,relayId:parseInt(document.getElementById('aCondRelayId'+i)?.value||0),
      sensorId:parseInt(document.getElementById('aCondSensorId'+i)?.value||0),
      thresh1:parseFloat(document.getElementById('aCondT1_'+i)?.value||0),
      thresh2:parseFloat(document.getElementById('aCondT2_'+i)?.value||100),
      hourStart:parseInt(timeS[0]),minuteStart:parseInt(timeS[1]),
      hourEnd:parseInt(timeE[0]),minuteEnd:parseInt(timeE[1]),
      weekdayMask,offlineMinutes:parseInt(document.getElementById('aCondOffMin'+i)?.value||10),
      negate:document.getElementById('aCondNot'+i)?.checked||false
    });
  }
  const actions=[];
  for(let i=0;i<autoActCount;i++){
    if(!document.getElementById('autoAct'+i))continue;
    const type=parseInt(document.getElementById('aActType'+i).value);
    actions.push({
      type,targetId:parseInt(document.getElementById('aActTargetSel'+i)?.value||0),
      durationMs:(parseInt(document.getElementById('aActDurVal'+i)?.value||30))*1000,
      smsText:document.getElementById('aActSMSText'+i)?.value||'',
      pulseOnMs:parseInt(document.getElementById('aActPON'+i)?.value||500),
      pulseOffMs:parseInt(document.getElementById('aActPOFF'+i)?.value||500),
      pulseCount:parseInt(document.getElementById('aActPCnt'+i)?.value||3),
      delayBeforeMs:parseInt(document.getElementById('aActDelay'+i)?.value||0)
    });
  }
  const hysteresis={
    enabled:document.getElementById('autoHystEnabled').checked,
    sensorId:parseInt(document.getElementById('autoHystSensor').value||0),
    relayId:parseInt(document.getElementById('autoHystRelay').value||0),
    onThreshold:parseFloat(document.getElementById('autoHystOn').value||30),
    offThreshold:parseFloat(document.getElementById('autoHystOff').value||27)
  };
  if(!conditions.length){toast('حداقل یک شرط','warning');return;}
  if(!actions.length){toast('حداقل یک عمل','warning');return;}
  const payload={
    id:id===-1?-1:id,
    name:document.getElementById('autoName').value.trim()||'اتوماسیون',
    active:true,logicOp:parseInt(document.getElementById('autoLogicOp').value),
    conditions,actions,hysteresis,
    cooldownEnabled:document.getElementById('autoCoolEnabled').checked,
    cooldownMinutes:parseInt(document.getElementById('autoCoolMin').value||5)
  };
  await api('/api/automations/save','POST',payload);
  toast('اتوماسیون ذخیره شد ✓');
  closeModal('autoModal');
  loadAutomations();
}
async function editAuto(id){
  const list=await api('/api/automations');
  const a=list.find(x=>x.id===id);
  if(a)openAutoModal(a);
}
async function toggleAuto(id){
  const d=await api('/api/automations/toggle','POST',{id});
  toast(d.active?'فعال شد ✅':'متوقف شد ⏸️','info');
  loadAutomations();
}
async function testAuto(id){
  const d=await api('/api/automations/test','POST',{id});
  toast(`تست: eval=${d.eval} | اعمال=${d.actionsRun}`,'info');
}
async function deleteAuto(id){
  showConfirm('این اتوماسیون حذف شود؟','حذف',async()=>{
    await api('/api/automations/delete','POST',{id});
    toast('حذف شد','info');loadAutomations();
  });
}

// ══════════════════════════════════════════════
//  RF BUTTONS
// ══════════════════════════════════════════════
async function startLearn(){
  S.learnCode=0;
  document.getElementById('learnBox').style.display='none';
  document.getElementById('learnActive').style.display='block';
  document.getElementById('learnResult').style.display='none';
  await api('/api/rf/learn','POST');
  let t=20;
  document.getElementById('learnCountdown').textContent=t;
  S.learnTimer=setInterval(async()=>{
    t--;document.getElementById('learnCountdown').textContent=t;
    if(t<=0){clearInterval(S.learnTimer);cancelLearn();toast('زمان تمام شد','warning');return;}
    try{
      const d=await api('/api/rf/learned');
      if(d.ready===true&&d.code>0){
        clearInterval(S.learnTimer);
        S.learnCode=d.code;S.learnProto=d.protocol;S.learnBits=d.bitLength;
        document.getElementById('learnActive').style.display='none';
        document.getElementById('learnResult').style.display='block';
        document.getElementById('learnCode').textContent=
          '0x'+d.code.toString(16).toUpperCase().padStart(8,'0')+
          ` (P${d.protocol}, ${d.bitLength}bit)`;
        await loadActionDropdowns();
        toast('کد دریافت شد ✓');
      }
    }catch(e){}
  },1000);
}
async function cancelLearn(){
  clearInterval(S.learnTimer);
  document.getElementById('learnBox').style.display='block';
  document.getElementById('learnActive').style.display='none';
  document.getElementById('learnResult').style.display='none';
  await api('/api/rf/cancel','POST');
}
async function loadActionDropdowns(){
  const scenes=await api('/api/scenes');
  const relayOpts='<option value="0">رله ۱</option><option value="1">رله ۲</option><option value="2">رله ۳</option><option value="3">رله ۴</option>';
  let sceneOpts='<option value="-1">— انتخاب —</option>';
  for(const s of scenes)sceneOpts+=`<option value="${s.id}">${s.name}</option>`;
  ['s','d','l'].forEach(p=>{
    const sel=document.getElementById(p+'t');
    sel.innerHTML=relayOpts;
    sel.dataset.relayOpts=relayOpts;
    sel.dataset.sceneOpts=sceneOpts;
    updateActionUI(p);
  });
}
function updateActionUI(p){
  const act=parseInt(document.getElementById(p+'a').value);
  const sel=document.getElementById(p+'t');
  if(act===1){sel.innerHTML=sel.dataset.sceneOpts||'';sel.style.display='block';}
  else if(act>=2&&act<=4){sel.innerHTML=sel.dataset.relayOpts||'';sel.style.display='block';}
  else{sel.style.display='none';}
}
async function saveBtn(){
  const name=document.getElementById('btnName').value.trim()||'دکمه';
  const getAT=p=>({action:parseInt(document.getElementById(p+'a').value),target:parseInt(document.getElementById(p+'t').value||0)});
  const sv=getAT('s'),dv=getAT('d'),lv=getAT('l');
  await api('/api/rf/save','POST',{
    code:S.learnCode,protocol:S.learnProto,bitLength:S.learnBits,name,
    singleAction:sv.action,singleTarget:sv.target,
    doubleAction:dv.action,doubleTarget:dv.target,
    longAction:lv.action,longTarget:lv.target,
    tripleAction:0,tripleTarget:0
  });
  toast('دکمه ذخیره شد');
  cancelLearn();loadRF();
}
async function loadRF(){
  const btns=await api('/api/rf/buttons');
  document.getElementById('rfCounter').textContent=btns.length+' دکمه';
  if(!btns.length){
    document.getElementById('rfList').innerHTML='<div class="empty"><span class="empty-icon">📡</span><p>هیچ دکمه‌ای ثبت نشده</p></div>';
    return;
  }
  let h='';
  for(const b of btns){
    const tags=[
      b.singleAction?`<span class="tag tag-single">1× ${actionLabels[b.singleAction]}</span>`:'',
      b.doubleAction?`<span class="tag tag-double">2× ${actionLabels[b.doubleAction]}</span>`:'',
      b.longAction?`<span class="tag tag-long">⏱ ${actionLabels[b.longAction]}</span>`:'',
    ].join('');
    h+=`<div class="list-item">
      <div class="item-icon">📡</div>
      <div class="item-body">
        <div class="item-title">${b.name}</div>
        <div style="font-family:monospace;font-size:10px;color:var(--primary);margin-bottom:4px">
          0x${b.code.toString(16).toUpperCase().padStart(8,'0')}</div>
        <div>${tags}</div>
      </div>
      <div class="item-actions">
        <button class="btn btn-danger btn-sm" onclick="delRF(${b.code})">🗑️</button>
      </div>
    </div>`;
  }
  document.getElementById('rfList').innerHTML=h;
}
async function delRF(code){
  showConfirm('دکمه حذف شود؟','حذف',async()=>{
    await api('/api/rf/delete','POST',{code});
    toast('حذف شد','info');loadRF();
  });
}

// ══════════════════════════════════════════════
//  SCENES
// ══════════════════════════════════════════════
async function openSceneModal(){
  S.stepCount=0;
  document.getElementById('stepList').innerHTML='';
  document.getElementById('sceneName').value='';
  document.getElementById('sceneSeq').checked=false;
  document.getElementById('sceneTimeEnabled').checked=false;
  document.getElementById('sceneTimePicker').style.display='none';
  document.querySelectorAll('.wday').forEach(cb=>cb.checked=true);
  addStep();openModal('sceneModal');
}
function addStep(){
  const id=S.stepCount++;
  const h=`<div class="step-card" id="step${id}">
    <div class="step-num">${id+1}</div>
    <div class="step-grid">
      <div class="form-group" style="margin:0"><label class="form-label">رله:</label>
        <select id="sr${id}"><option value="0">رله ۱</option><option value="1">رله ۲</option><option value="2">رله ۳</option><option value="3">رله ۴</option></select></div>
      <div class="form-group" style="margin:0"><label class="form-label">عملکرد:</label>
        <select id="sa${id}"><option value="0">—</option><option value="1">خاموش</option><option value="2">روشن</option><option value="3">روشن زماندار</option></select></div>
    </div>
    <div class="step-grid">
      <div class="form-group" style="margin:0"><label class="form-label">مدت (ثانیه):</label>
        <input type="number" id="sd${id}" value="0" min="0"></div>
      <div class="form-group" style="margin:0"><label class="form-label">تأخیر قبل (ثانیه):</label>
        <input type="number" id="sdelay${id}" value="0" min="0"></div>
    </div>
    ${id>0?`<button class="btn btn-danger btn-sm" onclick="document.getElementById('step${id}').remove()" style="margin-top:6px;width:100%">🗑️ حذف</button>`:''}
  </div>`;
  document.getElementById('stepList').insertAdjacentHTML('beforeend',h);
}
async function saveScene(){
  const name=document.getElementById('sceneName').value.trim()||'سناریو';
  const seq=document.getElementById('sceneSeq').checked;
  const timeEnabled=document.getElementById('sceneTimeEnabled').checked;
  const triggerHour=parseInt(document.getElementById('sceneTriggerHour').value)||0;
  const triggerMinute=parseInt(document.getElementById('sceneTriggerMinute').value)||0;
  const repeatInterval=parseInt(document.getElementById('sceneRepeatInterval').value)||0;
  let weekdayMask=0;
  document.querySelectorAll('.wday').forEach(cb=>{if(cb.checked)weekdayMask|=(1<<parseInt(cb.value));});
  if(weekdayMask===0)weekdayMask=0x7F;
  const steps=[];
  for(let i=0;i<S.stepCount;i++){
    const el=document.getElementById('step'+i);
    if(!el)continue;
    steps.push({
      relay:parseInt(document.getElementById('sr'+i).value),
      action:parseInt(document.getElementById('sa'+i).value),
      duration:parseInt(document.getElementById('sd'+i).value)||0,
      delayBefore:parseInt(document.getElementById('sdelay'+i).value)||0
    });
  }
  if(!steps.length){toast('حداقل یک مرحله','warning');return;}
  await api('/api/scenes/save','POST',{name,isSequential:seq,timeEnabled,triggerHour,triggerMinute,weekdayMask,repeatInterval,steps});
  toast('سناریو ذخیره شد');closeModal('sceneModal');loadScenes();
}
async function loadScenes(){
  const scenes=await api('/api/scenes');
  document.getElementById('sceneCounter').textContent=scenes.length+' سناریو';
  if(!scenes.length){
    document.getElementById('sceneList').innerHTML='<div class="empty"><span class="empty-icon">🎬</span><p>سناریویی تعریف نشده</p></div>';
    return;
  }
  const acts=['—','خاموش','روشن','⏱ زماندار'];
  let h='';
  for(const s of scenes){
    const stepsHtml=s.steps.map(st=>
      `<span class="tag tag-relay">رله${st.relay+1}:${acts[st.action]||'–'}${st.action===3?' '+st.duration+'s':''}${st.delayBefore>0?' +'+st.delayBefore+'s':''}</span>`
    ).join('');
    const timeTag=s.timeEnabled?`<span class="tag tag-single">⏰ ${String(s.triggerHour).padStart(2,'0')}:${String(s.triggerMinute).padStart(2,'0')}</span>`:'';
    h+=`<div class="list-item" style="align-items:flex-start">
      <div class="item-icon">🎬</div>
      <div class="item-body">
        <div class="item-title">${s.name}${s.isSequential?'<span class="tag tag-scene">مرحله‌ای</span>':''}${timeTag}</div>
        <div style="margin-top:4px">${stepsHtml}</div>
      </div>
      <div class="item-actions" style="flex-direction:column;gap:4px">
        <button class="btn btn-success btn-sm" onclick="runScene(${s.id})">▶️</button>
        <button class="btn btn-danger btn-sm" onclick="delScene(${s.id})">🗑️</button>
      </div>
    </div>`;
  }
  document.getElementById('sceneList').innerHTML=h;
}
async function runScene(id){
  await api('/api/scenes/run','POST',{id});
  toast('سناریو اجرا شد ▶️','info');
}
async function delScene(id){
  showConfirm('این سناریو حذف شود؟','حذف',async()=>{
    await api('/api/scenes/delete','POST',{id});
    toast('حذف شد','info');loadScenes();
  });
}

// ══════════════════════════════════════════════
//  COMBOS
// ══════════════════════════════════════════════
async function openComboModal(){
  const btns=await api('/api/rf/buttons');
  const scenes=await api('/api/scenes');
  if(btns.length<2){toast('حداقل ۲ دکمه ثبت شده نیاز است','warning');return;}
  let bOpts='';
  for(const b of btns)bOpts+=`<option value="${b.code}">${b.name}</option>`;
  document.getElementById('comboCode1').innerHTML=bOpts;
  document.getElementById('comboCode2').innerHTML=bOpts;
  if(btns.length>1)document.getElementById('comboCode2').selectedIndex=1;
  let sOpts='<option value="-1">—</option>';
  for(const s of scenes)sOpts+=`<option value="${s.id}">${s.name}</option>`;
  window._comboSceneOpts=sOpts;
  window._comboRelayOpts='<option value="0">رله ۱</option><option value="1">رله ۲</option><option value="2">رله ۳</option><option value="3">رله ۴</option>';
  updateComboUI();openModal('comboModal');
}
function updateComboUI(){
  const act=parseInt(document.getElementById('comboAction').value);
  const sel=document.getElementById('comboTarget');
  const grp=document.getElementById('comboTargetGroup');
  if(act===5){grp.style.display='none';return;}
  grp.style.display='block';
  sel.innerHTML=act===1?(window._comboSceneOpts||''):(window._comboRelayOpts||'');
}
async function saveCombo(){
  const name=document.getElementById('comboName').value.trim()||'ترکیب';
  const code1=parseInt(document.getElementById('comboCode1').value);
  const code2=parseInt(document.getElementById('comboCode2').value);
  const action=parseInt(document.getElementById('comboAction').value);
  const target=parseInt(document.getElementById('comboTarget').value||0);
  if(code1===code2){toast('دو دکمه باید متفاوت باشند','error');return;}
  await api('/api/combos/save','POST',{name,code1,code2,actionType:action,actionId:target});
  toast('ترکیب ذخیره شد');closeModal('comboModal');loadCombos();
}
async function loadCombos(){
  const combos=await api('/api/combos');
  if(!combos.length){
    document.getElementById('comboList').innerHTML='<div class="empty"><span class="empty-icon">🔗</span><p>ترکیبی تعریف نشده</p></div>';
    return;
  }
  let h='';
  for(const c of combos){
    h+=`<div class="list-item">
      <div class="item-icon">🔗</div>
      <div class="item-body">
        <div class="item-title">${c.name}</div>
        <div class="item-sub">0x${c.code1.toString(16).toUpperCase()} + 0x${c.code2.toString(16).toUpperCase()}<br>عملکرد: ${actionLabels[c.actionType]||'–'}</div>
      </div>
      <div class="item-actions">
        <button class="btn btn-danger btn-sm" onclick="delCombo(${c.code1},${c.code2})">🗑️</button>
      </div>
    </div>`;
  }
  document.getElementById('comboList').innerHTML=h;
}
async function delCombo(c1,c2){
  showConfirm('این ترکیب حذف شود؟','حذف',async()=>{
    await api('/api/combos/delete','POST',{code1:c1,code2:c2});
    toast('حذف شد','info');loadCombos();
  });
}

// ══════════════════════════════════════════════
//  SETTINGS
// ══════════════════════════════════════════════
async function loadSettings(){
  let relaySettings=[];
  try{relaySettings=await api('/api/relay/settings');}catch(e){}
  let h='';
  for(let i=0;i<4;i++){
    const rs=relaySettings[i]||{};
    h+=`<div class="setting-row">
      <div class="setting-info">
        <div class="setting-title">${relayNames[i]}</div>
        <div class="setting-desc">منطق معکوس (Active LOW)</div>
      </div>
      <div class="toggle-switch ${rs.logic?'on':''}" id="relayLogic${i}" onclick="toggleRelayLogic(${i})"></div>
    </div>
    <div class="setting-row">
      <div class="setting-info">
        <div class="setting-title">${relayNames[i]} — SMS</div>
        <div class="setting-desc">اطلاع‌رسانی هنگام تغییر حالت</div>
      </div>
      <div class="toggle-switch ${rs.notifySMS?'on':''}" id="relayNotify${i}" onclick="toggleRelayNotify(${i})"></div>
    </div>`;
  }
  document.getElementById('relaySettings').innerHTML=h;
  loadPhones();loadLogs();
  api('/api/status').then(d=>{
    document.getElementById('sysInfo').innerHTML=`
    📱 شماره‌های مجاز: ${d.phoneCount||0}<br>
    📡 RF Buttons: ${d.rfCount}<br>
    🎬 Scenes: ${d.sceneCount}<br>
    🤖 Automations: ${d.automationCount||0}<br>
    ⏰ زمان: ${d.time||'--'}<br>
    📶 Signal: ${d.signal}<br>
    🔋 Free Heap: ${Math.round((d.freeHeap||0)/1024)}K`;
  });
}
async function toggleRelayLogic(i){
  const el=document.getElementById('relayLogic'+i);
  const notifyEl=document.getElementById('relayNotify'+i);
  const isOn=el.classList.toggle('on');
  const notifySMS=notifyEl?notifyEl.classList.contains('on'):false;
  try{
    await api('/api/relay/settings/save','POST',{index:i,logic:isOn,notifySMS});
    toast(relayNames[i]+(isOn?' معکوس فعال':' معکوس غیرفعال'),'info');
  }catch(e){el.classList.toggle('on',!isOn);}
}
async function toggleRelayNotify(i){
  const el=document.getElementById('relayNotify'+i);
  const logicEl=document.getElementById('relayLogic'+i);
  const isOn=el.classList.toggle('on');
  const logic=logicEl?logicEl.classList.contains('on'):false;
  try{
    await api('/api/relay/settings/save','POST',{index:i,logic,notifySMS:isOn});
    toast(relayNames[i]+(isOn?' SMS فعال':' SMS غیرفعال'),'info');
  }catch(e){el.classList.toggle('on',!isOn);}
}
async function loadPhones(){
  const phones=await api('/api/phones');
  document.getElementById('phoneCounter').textContent=phones.length+' شماره';
  if(!phones.length){
    document.getElementById('phoneList').innerHTML='<div class="empty"><span class="empty-icon">📱</span><p>همه شماره‌ها مجاز</p></div>';
    return;
  }
  let h='';
  phones.forEach((p,i)=>{
    h+=`<div class="list-item">
      <div class="item-icon">📱</div>
      <div class="item-body"><div class="item-title" style="font-family:monospace">${p}</div></div>
      <div class="item-actions">
        <button class="btn btn-danger btn-sm" onclick="delPhone(${i})">🗑️</button>
      </div>
    </div>`;
  });
  document.getElementById('phoneList').innerHTML=h;
}
async function addPhone(){
  const num=document.getElementById('newPhone').value.trim();
  if(!num){toast('شماره را وارد کنید','warning');return;}
  if(num.length<8){toast('شماره نامعتبر','error');return;}
  await api('/api/phones/save','POST',{number:num});
  document.getElementById('newPhone').value='';
  toast('شماره اضافه شد ✓');loadPhones();
}
async function delPhone(idx){
  showConfirm('این شماره حذف شود؟','حذف',async()=>{
    await api('/api/phones/delete','POST',{index:idx});
    toast('حذف شد','info');loadPhones();
  });
}
async function loadLogs(){
  const logs=await api('/api/logs');
  document.getElementById('logCounter').textContent=logs.length+' رویداد';
  if(!logs.length){
    document.getElementById('logList').innerHTML='<div class="empty"><span class="empty-icon">📋</span><p>لاگی وجود ندارد</p></div>';
    return;
  }
  const types=['relay','scene','rf','sms','sys'];
  const typeLabels=['رله','سناریو','ریموت','پیامک','سیستم'];
  const levelIcons=['ℹ️','⚠️','❌'];
  const levelClasses=['','warn','err'];
  let h='';
  logs.forEach(log=>{
    const sec=log.time;const min=Math.floor(sec/60);const hr=Math.floor(min/60);
    const timeStr=hr>0?`${hr}h ${min%60}m ago`:min>0?`${min}m ${sec%60}s ago`:`${sec}s ago`;
    h+=`<div class="log-item ${levelClasses[log.level]}">
      <div class="log-time">${levelIcons[log.level]} ${timeStr}</div>
      <span class="log-type ${types[log.type]}">${typeLabels[log.type]}</span> ${log.msg}
    </div>`;
  });
  document.getElementById('logList').innerHTML=h;
}
async function clearLogs(){
  showConfirm('تمام لاگ‌ها پاک شوند؟','پاک',async()=>{
    await api('/api/logs/clear','POST');toast('لاگ‌ها پاک شدند','info');loadLogs();
  });
}
async function gsmSoftReset(){
  showConfirm('ریست نرم GSM؟','ریست',async()=>{
    await api('/api/reset/soft','POST');toast('ریست نرم انجام شد','info');
  });
}
async function gsmHardReset(){
  showConfirm('⚡ ریست سخت GSM؟','ریست',async()=>{
    await api('/api/reset/hard','POST');toast('ریست سخت در حال انجام...','warning');
  });
}
async function clearAllData(){
  showConfirm('⚠️ همه تنظیمات پاک شوند؟','پاک کردن',async()=>{
    await api('/api/clear','POST');toast('داده‌ها پاک شدند. دستگاه را ریست کنید.','warning',5000);
  });
}

// ══════════════════════════════════════════════
//  WIFI / OTA
// ══════════════════════════════════════════════
async function connectWifi(){
  const ssid=document.getElementById('wifiSSID').value.trim();
  const pass=document.getElementById('wifiPass').value;
  const statusEl=document.getElementById('wifiStatus');
  const btn=document.getElementById('wifiConnectBtn');
  if(!ssid){toast('SSID را وارد کنید','warning');return;}
  btn.disabled=true;
  statusEl.style.color='var(--warning)';
  statusEl.textContent='⏳ در حال اتصال...';
  try{
    await api('/api/wifi/connect','POST',{ssid,password:pass});
    let attempts=0;
    const poll=setInterval(async()=>{
      attempts++;
      try{
        const d=await api('/api/wifi/status');
        if(d.connected){
          clearInterval(poll);
          statusEl.style.color='var(--success)';
          statusEl.textContent=`✅ متصل شد: ${d.ip}`;
          toast('اتصال موفق! بررسی بروزرسانی...','success');
          btn.disabled=false;
          setTimeout(()=>checkOTA(),1500);
        }else if(d.failed||attempts>20){
          clearInterval(poll);
          statusEl.style.color='var(--danger)';
          statusEl.textContent='❌ اتصال ناموفق';
          btn.disabled=false;
          toast('اتصال WiFi ناموفق','error');
        }else{
          statusEl.textContent=`⏳ در حال اتصال... (${attempts*2}s)`;
        }
      }catch(e){}
    },2000);
  }catch(e){
    statusEl.style.color='var(--danger)';
    statusEl.textContent='❌ خطا';
    btn.disabled=false;
  }
}
async function checkOTA(){
  const statusEl=document.getElementById('wifiStatus');
  statusEl.style.color='var(--info)';
  statusEl.textContent='🔍 بررسی بروزرسانی...';
  try{await api('/api/ota/check','POST');}catch(e){return;}
  const poll=setInterval(async()=>{
    try{
      const d=await api('/api/ota/status');
      switch(d.status){
        case 'checking':statusEl.textContent='🔍 بررسی سرور...';break;
        case 'downloading':statusEl.textContent=`⬇️ دانلود ${d.newVersion}: ${d.progress}%`;break;
        case 'success':clearInterval(poll);statusEl.style.color='var(--success)';
          statusEl.textContent='✅ نصب شد — در حال ریست...';
          toast('بروزرسانی موفق!','success',6000);break;
        case 'up_to_date':clearInterval(poll);statusEl.style.color='var(--success)';
          statusEl.textContent=`✅ نسخه ${d.current} به‌روز است`;
          toast('دستگاه به‌روز است ✓','success');break;
        case 'failed':clearInterval(poll);statusEl.style.color='var(--danger)';
          statusEl.textContent='❌ '+(d.message||'خطا');
          toast('خطا: '+d.message,'error');break;
      }
    }catch(e){clearInterval(poll);}
  },1500);
}

// ══════════════════════════════════════════════
//  CLOCK
// ══════════════════════════════════════════════
function updateClock(){
  const now=new Date();
  document.getElementById('headerTime').textContent=
    now.getHours().toString().padStart(2,'0')+':'+now.getMinutes().toString().padStart(2,'0');
  const secs=Math.floor((Date.now()-S.startTime)/1000);
  const h=Math.floor(secs/3600),m=Math.floor((secs%3600)/60),s=secs%60;
  document.getElementById('headerUptime').textContent=
    h>0?`${h}h ${m}m`:m>0?`${m}m ${s}s`:`${s}s`;
}
setInterval(updateClock,1000);
updateClock();

// ══════════════════════════════════════════════
//  INIT
// ══════════════════════════════════════════════
setTimeout(()=>{
  const w=100/8;
  document.getElementById('navIndicator').style.cssText=`width:${w}%;right:0%;`;
  buildProfileGrid();
},100);

setInterval(()=>{if(S.currentPage===0)doRefresh();},10000);
doRefresh();
// ══════════════════════════════════════════════
//  PHONE MANAGEMENT (مدیریت شماره‌های مجاز - اختصاصی صفحه ۶)
// ══════════════════════════════════════════════
let editingPhoneId = null; // متغیر برای تشخیص وضعیت ادیت یا اضافه کردن جدید

// ۱. تابع بارگذاری لیست شماره‌ها و بروزرسانی شمارنده
async function loadPhones() {
  try {
    const phones = await api('/api/phones');
    const listEl = document.getElementById('phoneList');
    const counterEl = document.getElementById('phoneCounter');
    
    // بروزرسانی تعداد شماره‌ها در بالای کارت
    if (counterEl) counterEl.textContent = phones ? phones.length : 0;
    if (!listEl) return;
    
    listEl.innerHTML = '';
    
    // اگر لیستی وجود نداشت، نمایش حالت پیش‌فرض شما
    if (!phones || phones.length === 0) {
      listEl.innerHTML = '<div class="empty"><span class="empty-icon">📱</span><p>همه شماره‌ها مجاز</p></div>';
      return;
    }

    // رندر کردن تک‌تک شماره‌ها با ساختار کلاس‌های قالب شما
    phones.forEach((p, index) => {
      const currentId = p.id !== undefined ? p.id : index;
      const phoneNumber = p.phone || p.number || '';
      const name = p.name || `مخاطب ${index + 1}`;

      listEl.innerHTML += `
        <div class="list-item">
          <div class="item-icon">📱</div>
          <div class="item-body">
            <div class="item-title" style="font-size:13px;">${name}</div>
            <div class="item-sub" style="direction:ltr; text-align:right; font-family:monospace;">${phoneNumber}</div>
          </div>
          <div class="item-actions">
            <button class="btn btn-warning btn-sm" onclick="editPhone('${currentId}', '${phoneNumber}')">✏️</button>
            <button class="btn btn-danger btn-sm" onclick="deletePhone('${currentId}', '${phoneNumber}')">🗑️</button>
          </div>
        </div>
      `;
    });
  } catch (e) {
    console.error("Error loading phones:", e);
  }
}

// ۲. تابع اضافه کردن یا ذخیره ادیت (متصل به دکمه ➕ در HTML شما)
async function addPhone() {
  const input = document.getElementById('newPhone');
  if (!input || !input.value.trim()) {
    toast('لطفاً شماره تلفن را وارد کنید', 'warning');
    return;
  }

  const payload = {
    phone: input.value.trim()
  };
  
  // اگر متغیر ادیت پر باشد، یعنی در حال ویرایش هستیم و ID را ارسال می‌کنیم
  if (editingPhoneId !== null) {
    payload.id = parseInt(editingPhoneId);
  }

  try {
    await api('/api/phone/save', 'POST', payload);
    toast(editingPhoneId !== null ? 'شماره با موفقیت ویرایش شد ✓' : 'شماره با موفقیت اضافه شد ✓', 'success');
    
    // ریست کردن فرم به حالت عادی
    input.value = '';
    editingPhoneId = null;
    
    // تغییر مجدد آیکون دکمه به پلاس (➕)
    const btn = input.nextElementSibling;
    if (btn) btn.innerHTML = '➕';
    
    loadPhones(); // لود مجدد لیست
  } catch (e) {
    // خطا به طور خودکار توسط تابع api سیستم شما مدیریت می‌شود
  }
}

// ۳. تابع حذف شماره (متصل به دکمه سطل زباله لیست)
async function deletePhone(id, phoneNumber) {
  showConfirm(`آیا از حذف شماره ${phoneNumber} مطمئن هستید؟`, 'حذف', async () => {
    try {
      await api('/api/phone/delete', 'POST', { id: parseInt(id), phone: phoneNumber });
      toast('شماره با موفقیت حذف شد ✓', 'success');
      loadPhones();
    } catch (e) {}
  });
}

// ۴. تابع هدایت شماره به باکس ورودی جهت ادیت
function editPhone(id, phoneNumber) {
  const input = document.getElementById('newPhone');
  if (!input) return;
  
  input.value = phoneNumber; // قرار دادن شماره در باکس متنی
  editingPhoneId = id;       // ذخیره شناسه برای فهماندن وضعیت به تابع addPhone
  
  // تغییر ظاهر دکمه پلاس به دیسکت (💾) جهت راهنمایی کاربر برای ذخیره
  const btn = input.nextElementSibling;
  if (btn) btn.innerHTML = '💾';
  
  toast('شماره برای ویرایش بارگذاری شد. تغییرات را اعمال و روی دکمه ذخیره کلیک کنید.', 'info');
}
</script>
</body>
</html>)rawliteral";