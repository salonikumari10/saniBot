//  SaniBot State 
var saniMode = 'AUTO';
var mistOn = false;
var uvOn   = false;

function selectModeAndClose(m) {
    document.getElementById('popup-overlay').style.display = 'none';
    setMode(m);
}
      
function setMode(m) {
    saniMode = m;
    fetch(location.origin + '/?sani=' + m + ';stop');
    document.getElementById('btnAuto').className   = 'mode-btn' + (m === 'AUTO'   ? ' auto-on'   : '');
    document.getElementById('btnManual').className = 'mode-btn' + (m === 'MANUAL' ? ' manual-on' : '');
    document.getElementById('modeDisp').textContent = m;
    ['bF','bB','bL','bR'].forEach(function(id){
        document.getElementById(id).disabled = (m === 'AUTO');
    });
}

function sendSani(cmd) {
    if (saniMode !== 'MANUAL') {
        var w = document.getElementById('warnBox');
        w.style.display = 'block';
        setTimeout(function(){ w.style.display = 'none'; }, 2000);
        return;
    }
    fetch(location.origin + '/?sani=' + cmd + ';stop');
    document.getElementById('cmdDisp').textContent = cmd;
}

function setSpeed(val) {
    document.getElementById('spdVal').textContent = val;
    fetch(location.origin + '/?sani=SPD:' + val + ';stop');
}

function toggleRelay(which) {
    if (which === 'MIST') {
        mistOn = !mistOn;
        fetch(location.origin + '/?sani=MIST:' + (mistOn?'1':'0') + ';stop');
        document.getElementById('mistBtn').className     = 'relay-btn' + (mistOn ? ' mist-on' : '');
        document.getElementById('mistState').textContent = mistOn ? 'ON' : 'OFF';
    } else {
        uvOn = !uvOn;
        fetch(location.origin + '/?sani=UV:' + (uvOn?'1':'0') + ';stop');
        document.getElementById('uvBtn').className     = 'relay-btn' + (uvOn ? ' uv-on' : '');
        document.getElementById('uvState').textContent = uvOn ? 'ON' : 'OFF';
    }
}

function pollSani() {
    fetch(location.origin + '/?sanistatus=1;stop')
    .then(function(r){ return r.text(); })
    .then(function(t) {
        var pairs = t.split(':');
        var data = {};
        for (var i = 0; i < pairs.length - 1; i += 2) data[pairs[i]] = pairs[i+1];
        if (data.FRONT === undefined) return;
            
        var fd = parseInt(data.FRONT);
        var rd = parseInt(data.RIGHT);
        var fEl = document.getElementById('fVal');
        var rEl = document.getElementById('rVal');
            
        fEl.textContent = (fd >= 999 ? '>400' : fd) + ' cm';
        rEl.textContent = (rd >= 999 ? '>400' : rd) + ' cm';
        fEl.className = fd < 30 ? 'sensor-val near' : fd < 60 ? 'sensor-val mid' : 'sensor-val far';
        rEl.className = rd < 25 ? 'sensor-val near' : rd < 50 ? 'sensor-val mid' : 'sensor-val far';
        var obs = document.getElementById('obsDisp');
        if (fd < 30) { 
            obs.textContent = 'OBSTACLE'; obs.className = 's-val s-bad'; 
        } else { 
            obs.textContent = 'CLEAR';    obs.className = 's-val s-ok';  
        }
        
        if (data.CMD)  document.getElementById('cmdDisp').textContent = data.CMD;
        if (data.MODE && data.MODE !== saniMode) setMode(data.MODE);
            
        var newUV = (data.UV ? data.UV.trim() === '1' : false);
        var newMist = (data.MIST ? data.MIST.trim() === '1' : false);
        if (newMist !== mistOn) {
            mistOn = newMist;
            document.getElementById('mistBtn').className     = 'relay-btn' + (mistOn ? ' mist-on' : '');
            document.getElementById('mistState').textContent = mistOn ? 'ON' : 'OFF';
        }
        if (newUV !== uvOn) {
            uvOn = newUV;
            document.getElementById('uvBtn').className     = 'relay-btn' + (uvOn ? ' uv-on' : '');
            document.getElementById('uvState').textContent = uvOn ? 'ON' : 'OFF';
        }
        document.getElementById('connStatus').textContent = 'ONLINE';
    })   
    .catch(function(){
        document.getElementById('connStatus').textContent = 'RECONNECTING...';
    });
}

//  Keyboard Control
document.addEventListener('keydown', function(e) {
    if (saniMode !== 'MANUAL') return;
    var map = {ArrowUp:'FORWARD', ArrowDown:'BACKWARD', ArrowLeft:'LEFT', ArrowRight:'RIGHT'};
    if (map[e.key]) { 
        sendSani(map[e.key]); e.preventDefault(); 
    }
});

document.addEventListener('keyup', function(e) {
    if (['ArrowUp','ArrowDown','ArrowLeft','ArrowRight'].indexOf(e.key) >= 0) sendSani('STOP');
            
});

//  Object Detection 
var getStill    = document.getElementById('getStill');
var ShowImage   = document.getElementById('ShowImage');
var canvas      = document.getElementById('canvas');
var context     = canvas.getContext('2d');
var object      = document.getElementById('object');
var score       = document.getElementById('score');
var mirrorimage = document.getElementById('mirrorimage');
var count       = document.getElementById('count-badge');
var result      = document.getElementById('result');
var flash       = document.getElementById('flash');
var myTimer;
var restartCount = 0;
var Model;

getStill.onclick = function() {
    clearInterval(myTimer);
    myTimer = setInterval(function(){ error_handle(); }, 5000);
    ShowImage.src = location.origin + '/?getstill=' + Math.random();
    getStill.classList.add('active');
};

function error_handle() {
    restartCount++;
    clearInterval(myTimer);
    getStill.classList.remove('active');
    if (restartCount <= 2) {
        result.innerHTML = 'Get still error. Restart ESP32-CAM ' + restartCount;
        myTimer = setInterval(function(){ getStill.click(); }, 10000);
    } else {
        result.innerHTML = 'Get still error. Please check ESP32-CAM.';
    }
}

ShowImage.onload = function() {
    clearInterval(myTimer);
    restartCount = 0;
    canvas.setAttribute('width', ShowImage.width);
    canvas.setAttribute('height', ShowImage.height);
    if (mirrorimage.value == 1) {
        context.translate((canvas.width + ShowImage.width) / 2, 0);
        context.scale(-1, 1);
        context.drawImage(ShowImage, 0, 0, ShowImage.width, ShowImage.height);
        context.setTransform(1, 0, 0, 1, 0, 0);
    } else {
        context.drawImage(ShowImage, 0, 0, ShowImage.width, ShowImage.height);
    }
    if (Model) DetectImage();
};

document.getElementById('restart').onclick = function() {
    fetch(location.origin + '/?restart=stop');
};
document.getElementById('framesize').onclick = function() {
    fetch(location.origin + '/?framesize=' + this.value + ';stop');
};
flash.onchange = function() {
    document.getElementById('fv').textContent = this.value;
    fetch(location.origin + '/?flash=' + this.value + ';stop');
};
document.getElementById('quality').onclick = function() {
    document.getElementById('qv').textContent = this.value;
    fetch(location.origin + '/?quality=' + this.value + ';stop');
};
document.getElementById('brightness').onclick = function() {
    document.getElementById('bv').textContent = this.value;
    fetch(location.origin + '/?brightness=' + this.value + ';stop');
};

document.getElementById('contrast').onclick = function() {
    document.getElementById('cv').textContent = this.value;
    fetch(location.origin + '/?contrast=' + this.value + ';stop');
};

function ObjectDetect() {
    result.innerHTML = 'Model is loading...., please wait...';
    cocoSsd.load().then(function(m) {
        Model = m;
        result.innerHTML = 'Model ready!';
        getStill.style.display = 'block';
    });
}

function DetectImage() {
    Model.detect(canvas).then(function(Predictions) {
        var s = canvas.width > canvas.height ? canvas.width : canvas.height;
        var objectCount = 0;
        if (Predictions.length > 0) {
            result.innerHTML = '';
            for (var i = 0; i < Predictions.length; i++) {
                var x = Predictions[i].bbox[0], y = Predictions[i].bbox[1];
                var w = Predictions[i].bbox[2], h = Predictions[i].bbox[3];
                context.lineWidth   = Math.round(s / 200);
                context.strokeStyle = '#1a6b5c';
                context.beginPath(); context.rect(x, y, w, h); context.stroke();
                context.fillStyle = '#c45c1a';
                context.font      = Math.round(s / 30) + 'px sans-serif';
                context.fillText(Predictions[i].class, x, y);
                result.innerHTML += '[' + i + '] ' + Predictions[i].class + ', ' +
                Math.round(Predictions[i].score * 100) + '%, ' +
                Math.round(x) + ',' + Math.round(y) + '<br>';

                if (Predictions[i].class == object.value && Predictions[i].score >= score.value) objectCount++;
            }
            count.innerHTML = objectCount;
        } else {
            result.innerHTML = 'No object found';
            count.innerHTML  = '0';
        }
            
        if (objectCount > 0) {
            $.ajax({ url: location.origin + '/?detectCount=' + object.value + ';' + String(objectCount) + ';stop', async: false });
        }
        if (object.value === 'person') {
            if (objectCount > 0) {
                fetch(location.origin + '/?sani=UV:0;stop');
            } else {
                fetch(location.origin + '/?sani=UV:1;stop');
            }
        }
        try {
            document.createEvent('TouchEvent');
            setTimeout(function(){ getStill.click(); }, 250);
        } catch(e) {
            setTimeout(function(){ getStill.click(); }, 150);
        }
    });
}

//  Init 
window.onload = function() {
    ObjectDetect();
    setInterval(pollSani, 500);
};