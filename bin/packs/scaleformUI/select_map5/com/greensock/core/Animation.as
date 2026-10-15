class com.greensock.core.Animation
{
   var vars;
   var _duration;
   var _totalDuration;
   var _delay;
   var _timeScale;
   var _totalTime;
   var _time;
   var data;
   var _rawPrevTime;
   var _paused;
   var _reversed;
   var _timeline;
   var _gc;
   var _startTime;
   var _active;
   var timeline;
   var _onUpdate;
   var _dirty;
   var _pauseTime;
   var _initted;
   static var _rootTimeline;
   static var _rootFramesTimeline;
   static var version = "12.1.3";
   static var ticker = com.greensock.core.Animation._jumpStart(_root);
   static var _rootFrame = -1;
   static var _onTick = [];
   static var _tinyNum = 1e-10;
   function Animation(duration, vars)
   {
      this.vars = vars || {};
      this._duration = this._totalDuration = duration || 0;
      this._delay = Number(this.vars.delay) || 0;
      this._timeScale = 1;
      this._totalTime = this._time = 0;
      this.data = this.vars.data;
      this._rawPrevTime = -1;
      this._paused = false;
      if(com.greensock.core.Animation._rootTimeline == null)
      {
         if(com.greensock.core.Animation._rootFrame !== -1)
         {
            return;
         }
         com.greensock.core.Animation._rootFrame = 0;
         com.greensock.core.Animation._rootFramesTimeline = new com.greensock.core.SimpleTimeline();
         com.greensock.core.Animation._rootTimeline = new com.greensock.core.SimpleTimeline();
         com.greensock.core.Animation._rootTimeline._startTime = getTimer() / 1000;
         com.greensock.core.Animation._rootFramesTimeline._startTime = 0;
         com.greensock.core.Animation._rootTimeline._active = com.greensock.core.Animation._rootFramesTimeline._active = true;
         com.greensock.core.Animation._addTickListener("tick",com.greensock.core.Animation._updateRoot,com.greensock.core.Animation);
      }
      if(com.greensock.core.Animation.ticker.onEnterFrame !== com.greensock.core.Animation._tick)
      {
         com.greensock.core.Animation._jumpStart(_root);
      }
      var tl = !this.vars.useFrames ? com.greensock.core.Animation._rootTimeline : com.greensock.core.Animation._rootFramesTimeline;
      tl.add(this,tl._time);
      this._reversed = this.vars.reversed == true;
      if(this.vars.paused)
      {
         this.paused(true);
      }
   }
   function play(from, suppressEvents)
   {
      if(arguments.length)
      {
         this.seek(from,suppressEvents);
      }
      this.reversed(false);
      return this.paused(false);
   }
   function pause(atTime, suppressEvents)
   {
      if(arguments.length)
      {
         this.seek(atTime,suppressEvents);
      }
      return this.paused(true);
   }
   function resume(from, suppressEvents)
   {
      if(from != null)
      {
         this.seek(from,suppressEvents);
      }
      return this.paused(false);
   }
   function seek(time, suppressEvents)
   {
      return this.totalTime(Number(time),suppressEvents != false);
   }
   function restart(includeDelay, suppressEvents)
   {
      this.reversed(false);
      this.paused(false);
      return this.totalTime(!includeDelay ? 0 : - this._delay,suppressEvents != false,true);
   }
   function reverse(from, suppressEvents)
   {
      if(from != null)
      {
         this.seek(from || this.totalDuration(),suppressEvents);
      }
      this.reversed(true);
      return this.paused(false);
   }
   function render(time, suppressEvents, force)
   {
   }
   function invalidate()
   {
      return this;
   }
   function isActive()
   {
      var tl = this._timeline;
      var rawTime;
      return tl == null || !this._gc && !this._paused && tl.isActive() && (rawTime = tl.rawTime()) >= this._startTime && rawTime < this._startTime + this.totalDuration() / this._timeScale;
   }
   function _enabled(enabled, ignoreTimeline)
   {
      this._gc = !enabled;
      this._active = this.isActive();
      if(ignoreTimeline != true)
      {
         if(enabled && this.timeline == null)
         {
            this._timeline.add(this,this._startTime - this._delay);
         }
         else if(!enabled && this.timeline != null)
         {
            this._timeline._remove(this,true);
         }
      }
      return false;
   }
   function _kill(vars, target)
   {
      return this._enabled(false,false);
   }
   function kill(vars, target)
   {
      this._kill(vars,target);
      return this;
   }
   function _uncache(includeSelf)
   {
      var tween = !includeSelf ? this.timeline : this;
      while(tween)
      {
         tween._dirty = true;
         tween = tween.timeline;
      }
      return this;
   }
   static function _updateRoot()
   {
      com.greensock.core.Animation._rootFrame++;
      com.greensock.core.Animation._rootTimeline.render((getTimer() / 1000 - com.greensock.core.Animation._rootTimeline._startTime) * com.greensock.core.Animation._rootTimeline._timeScale,false,false);
      com.greensock.core.Animation._rootFramesTimeline.render((com.greensock.core.Animation._rootFrame - com.greensock.core.Animation._rootFramesTimeline._startTime) * com.greensock.core.Animation._rootFramesTimeline._timeScale,false,false);
   }
   static function _addTickListener(type, callback, scope, useParam, priority)
   {
      if(type === "tick")
      {
         priority = priority || 0;
         var i = com.greensock.core.Animation._onTick.length;
         var index = 0;
         var l;
         while(--i > -1)
         {
            if(l = com.greensock.core.Animation._onTick[i].c === callback)
            {
               com.greensock.core.Animation._onTick.splice(i,1);
            }
            else if(index === 0 && l.p < priority)
            {
               index = i + 1;
            }
         }
         com.greensock.core.Animation._onTick.splice(index,0,{c:callback,s:scope,up:useParam,p:priority});
      }
   }
   static function _removeTickListener(type, callback)
   {
      var i = com.greensock.core.Animation._onTick.length;
      while(--i > -1)
      {
         if(com.greensock.core.Animation._onTick[i].c === callback && type === "tick")
         {
            com.greensock.core.Animation._onTick.splice(i,1);
            return undefined;
         }
      }
   }
   static function _tick()
   {
      var i = com.greensock.core.Animation._onTick.length;
      var l;
      while(--i > -1)
      {
         if(l = com.greensock.core.Animation._onTick[i].up)
         {
            l.c.call(l.s,{type:"tick",target:com.greensock.core.Animation.ticker});
         }
         else
         {
            l.c.call(l.s);
         }
      }
   }
   static function _findSubloadedSWF(mc)
   {
      for(var p in mc)
      {
         if(typeof mc[p] == "movieclip")
         {
            if(mc[p]._url != _root._url && mc[p].getBytesLoaded() != undefined)
            {
               return mc[p];
            }
            if(com.greensock.core.Animation._findSubloadedSWF(mc[p]))
            {
               return com.greensock.core.Animation._findSubloadedSWF(mc[p]);
            }
         }
      }
      return undefined;
   }
   static function _jumpStart(root)
   {
      if(com.greensock.core.Animation.ticker != undefined)
      {
         com.greensock.core.Animation.ticker.removeMovieClip();
      }
      var mc = root.getBytesLoaded() != undefined ? root : com.greensock.core.Animation._findSubloadedSWF(root);
      var l = 999;
      while(mc.getInstanceAtDepth(l))
      {
         l++;
      }
      var tickerName = "_gsAnimation" + String(com.greensock.core.Animation.version).split(".").join("_");
      com.greensock.core.Animation.ticker = mc[tickerName] || mc.createEmptyMovieClip(tickerName,l);
      com.greensock.core.Animation.ticker.onEnterFrame = com.greensock.core.Animation._tick;
      com.greensock.core.Animation.ticker.addEventListener = com.greensock.core.Animation._addTickListener;
      com.greensock.core.Animation.ticker.removeEventListener = com.greensock.core.Animation._removeTickListener;
      com.greensock.core.Animation._rootTimeline._time = com.greensock.core.Animation._rootTimeline._totalTime = (getTimer() / 1000 - com.greensock.core.Animation._rootTimeline._startTime) * com.greensock.core.Animation._rootTimeline._timeScale;
      return com.greensock.core.Animation.ticker;
   }
   function _swapSelfInParams(params)
   {
      var i = params.length;
      var copy = params.concat();
      while(--i > -1)
      {
         if(params[i] === "{self}")
         {
            copy[i] = this;
         }
      }
      return copy;
   }
   function eventCallback(type, callback, params, scope)
   {
      if(type == null)
      {
         return null;
      }
      if(type.substr(0,2) === "on")
      {
         if(arguments.length === 1)
         {
            return this.vars[type];
         }
         if(callback == null)
         {
            delete this.vars[type];
         }
         else
         {
            this.vars[type] = callback;
            this.vars[type + "Params"] = !(params instanceof Array && params.join("").indexOf("{self}") !== -1) ? params : this._swapSelfInParams(params);
            this.vars[type + "Scope"] = scope;
         }
         if(type === "onUpdate")
         {
            this._onUpdate = callback;
         }
      }
      return this;
   }
   function delay(value)
   {
      if(!arguments.length)
      {
         return this._delay;
      }
      if(this._timeline.smoothChildTiming)
      {
         this.startTime(this._startTime + value - this._delay);
      }
      this._delay = value;
      return this;
   }
   function duration(value)
   {
      if(!arguments.length)
      {
         this._dirty = false;
         return this._duration;
      }
      this._duration = this._totalDuration = value;
      this._uncache(true);
      if(this._timeline.smoothChildTiming)
      {
         if(this._time > 0)
         {
            if(this._time < this._duration)
            {
               if(value != 0)
               {
                  this.totalTime(this._totalTime * (value / this._duration),true);
               }
            }
         }
      }
      return this;
   }
   function totalDuration(value)
   {
      this._dirty = false;
      return !!arguments.length ? this.duration(value) : this._totalDuration;
   }
   function time(value, suppressEvents)
   {
      if(!arguments.length)
      {
         return this._time;
      }
      if(this._dirty)
      {
         this.totalDuration();
      }
      if(value > this._duration)
      {
         value = this._duration;
      }
      return this.totalTime(value,suppressEvents);
   }
   function totalTime(time, suppressEvents, uncapped)
   {
      if(!arguments.length)
      {
         return this._totalTime;
      }
      if(this._timeline)
      {
         if(time < 0 && !uncapped)
         {
            time += this.totalDuration();
         }
         if(this._timeline.smoothChildTiming)
         {
            if(this._dirty)
            {
               this.totalDuration();
            }
            if(time > this._totalDuration && !uncapped)
            {
               time = this._totalDuration;
            }
            var tl = this._timeline;
            this._startTime = (!this._paused ? tl._time : this._pauseTime) - (!!this._reversed ? this._totalDuration - time : time) / this._timeScale;
            if(!this._timeline._dirty)
            {
               this._uncache(false);
            }
            if(tl._timeline != null)
            {
               while(tl._timeline)
               {
                  if(tl._timeline._time !== (tl._startTime + tl._totalTime) / tl._timeScale)
                  {
                     tl.totalTime(tl._totalTime,true);
                  }
                  tl = tl._timeline;
               }
            }
         }
         if(this._gc)
         {
            this._enabled(true,false);
         }
         if(this._totalTime !== time || this._duration === 0)
         {
            this.render(time,suppressEvents,false);
         }
      }
      return this;
   }
   function progress(value, suppressEvents)
   {
      return !!arguments.length ? this.totalTime(this.duration() * value,suppressEvents) : this._time / this.duration();
   }
   function totalProgress(value, suppressEvents)
   {
      return !!arguments.length ? this.totalTime(this.duration() * value,suppressEvents) : this._time / this.duration();
   }
   function startTime(value)
   {
      if(!arguments.length)
      {
         return this._startTime;
      }
      if(value != this._startTime)
      {
         this._startTime = value;
         if(this.timeline)
         {
            if(this.timeline._sortChildren)
            {
               this.timeline.add(this,value - this._delay);
            }
         }
      }
      return this;
   }
   function timeScale(value)
   {
      if(!arguments.length)
      {
         return this._timeScale;
      }
      value = value || 0.000001;
      if(this._timeline && this._timeline.smoothChildTiming)
      {
         var t = !(this._pauseTime || this._pauseTime == 0) ? this._timeline._totalTime : this._pauseTime;
         this._startTime = t - (t - this._startTime) * this._timeScale / value;
      }
      this._timeScale = value;
      return this._uncache(false);
   }
   function reversed(value)
   {
      if(!arguments.length)
      {
         return this._reversed;
      }
      if(value != this._reversed)
      {
         this._reversed = value;
         this.totalTime(!(this._timeline && !this._timeline.smoothChildTiming) ? this._totalTime : this.totalDuration() - this._totalTime,true);
      }
      return this;
   }
   function paused(value)
   {
      if(!arguments.length)
      {
         return this._paused;
      }
      if(value != this._paused)
      {
         if(this._timeline)
         {
            var raw = this._timeline.rawTime();
            var elapsed = raw - this._pauseTime;
            if(!value && this._timeline.smoothChildTiming)
            {
               this._startTime += elapsed;
               this._uncache(false);
            }
            this._pauseTime = !value ? NaN : raw;
            this._paused = value;
            this._active = Boolean(!value && this._totalTime > 0 && this._totalTime < this._totalDuration);
            if(!value && elapsed !== 0 && this._initted && this.duration() !== 0)
            {
               this.render(!this._timeline.smoothChildTiming ? (raw - this._startTime) / this._timeScale : this._totalTime,true,true);
            }
         }
      }
      if(this._gc && !value)
      {
         this._enabled(true,false);
      }
      return this;
   }
}
