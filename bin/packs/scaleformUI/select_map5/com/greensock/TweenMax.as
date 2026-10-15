class com.greensock.TweenMax extends com.greensock.TweenLite
{
   var _cycle;
   var _yoyo;
   var vars;
   var _repeat;
   var _repeatDelay;
   var _dirty;
   var ratio;
   var _startTime;
   var _timeline;
   var _gc;
   var _delay;
   var _initted;
   var _notifyPluginsOfEnabled;
   var _firstPT;
   var _time;
   var _duration;
   var _totalDuration;
   var _totalTime;
   var _ease;
   var _reversed;
   var _rawPrevTime;
   var _active;
   var _easeType;
   var _easePower;
   var _onUpdate;
   var _paused;
   var _startAt;
   static var version = "12.1.5";
   static var _activatedPlugins = com.greensock.plugins.TweenPlugin.activate([com.greensock.plugins.AutoAlphaPlugin,com.greensock.plugins.EndArrayPlugin,com.greensock.plugins.FramePlugin,com.greensock.plugins.RemoveTintPlugin,com.greensock.plugins.TintPlugin,com.greensock.plugins.VisiblePlugin,com.greensock.plugins.VolumePlugin,com.greensock.plugins.BevelFilterPlugin,com.greensock.plugins.BezierPlugin,com.greensock.plugins.BezierThroughPlugin,com.greensock.plugins.BlurFilterPlugin,com.greensock.plugins
   .ColorMatrixFilterPlugin,com.greensock.plugins.ColorTransformPlugin,com.greensock.plugins.DropShadowFilterPlugin,com.greensock.plugins.FrameLabelPlugin,com.greensock.plugins.GlowFilterPlugin,com.greensock.plugins.HexColorsPlugin,com.greensock.plugins.RoundPropsPlugin,com.greensock.plugins.ShortRotationPlugin]);
   static var killTweensOf = com.greensock.TweenLite.killTweensOf;
   static var killDelayedCallsTo = com.greensock.TweenLite.killTweensOf;
   static var getTweensOf = com.greensock.TweenLite.getTweensOf;
   static var ticker = com.greensock.core.Animation.ticker;
   static var allTo = com.greensock.TweenMax.staggerTo;
   static var allFrom = com.greensock.TweenMax.staggerFrom;
   static var allFromTo = com.greensock.TweenMax.staggerFromTo;
   function TweenMax(target, duration, vars)
   {
      super(target,duration,vars);
      this._cycle = 0;
      this._yoyo = this.vars.yoyo == true;
      this._repeat = this.vars.repeat || 0;
      this._repeatDelay = this.vars.repeatDelay || 0;
      this._dirty = true;
   }
   function invalidate()
   {
      this._yoyo = this.vars.yoyo == true;
      this._repeat = this.vars.repeat || 0;
      this._repeatDelay = this.vars.repeatDelay || 0;
      this._uncache(true);
      return super.invalidate();
   }
   function updateTo(vars, resetDuration)
   {
      var curRatio = this.ratio;
      if(resetDuration)
      {
         if(this._startTime < this._timeline._time)
         {
            this._startTime = this._timeline._time;
            this._uncache(false);
            if(this._gc)
            {
               this._enabled(true,false);
            }
            else
            {
               this._timeline.insert(this,this._startTime - this._delay);
            }
         }
      }
      for(var p in vars)
      {
         this.vars[p] = vars[p];
      }
      if(this._initted)
      {
         if(resetDuration)
         {
            this._initted = false;
         }
         else
         {
            if(this._gc)
            {
               this._enabled(true,false);
            }
            if(this._notifyPluginsOfEnabled && this._firstPT)
            {
               com.greensock.TweenLite._onPluginEvent("_onDisable",this);
            }
            if(this._time / this._duration > 0.998)
            {
               var prevTime = this._time;
               this.render(0,true,false);
               this._initted = false;
               this.render(prevTime,true,false);
            }
            else if(this._time > 0)
            {
               this._initted = false;
               this._init();
               var inv = 1 / (1 - curRatio);
               var pt = this._firstPT;
               var endValue;
               while(pt)
               {
                  endValue = pt.s + pt.c;
                  pt.c *= inv;
                  pt.s = endValue - pt.c;
                  pt = pt._next;
               }
            }
         }
      }
      return this;
   }
   function render(time, suppressEvents, force)
   {
      if(!this._initted)
      {
         if(this._duration === 0 && this.vars.repeat)
         {
            this.invalidate();
         }
      }
      var totalDur = !!this._dirty ? this.totalDuration() : this._totalDuration;
      var prevTime = this._time;
      var prevTotalTime = this._totalTime;
      var prevCycle = this._cycle;
      var isComplete;
      var callback;
      var pt;
      var rawPrevTime;
      if(time >= totalDur)
      {
         this._totalTime = totalDur;
         this._cycle = this._repeat;
         if(this._yoyo && (this._cycle & 1) !== 0)
         {
            this._time = 0;
            this.ratio = !this._ease._calcEnd ? 0 : this._ease.getRatio(0);
         }
         else
         {
            this._time = this._duration;
            this.ratio = !this._ease._calcEnd ? 1 : this._ease.getRatio(1);
         }
         if(!this._reversed)
         {
            isComplete = true;
            callback = "onComplete";
         }
         if(this._duration === 0)
         {
            rawPrevTime = this._rawPrevTime;
            if(this._startTime === this._timeline._duration)
            {
               time = 0;
            }
            if(time === 0 || rawPrevTime < 0 || rawPrevTime === com.greensock.core.Animation._tinyNum)
            {
               if(rawPrevTime !== time)
               {
                  force = true;
                  if(rawPrevTime > com.greensock.core.Animation._tinyNum)
                  {
                     callback = "onReverseComplete";
                  }
               }
            }
            this._rawPrevTime = rawPrevTime = !(!suppressEvents || time !== 0 || this._rawPrevTime === time) ? com.greensock.core.Animation._tinyNum : time;
         }
      }
      else if(time < 1e-7)
      {
         this._totalTime = this._time = this._cycle = 0;
         this.ratio = !this._ease._calcEnd ? 0 : this._ease.getRatio(0);
         if(prevTotalTime != 0 || this._duration == 0 && this._rawPrevTime > 0 && this._rawPrevTime !== com.greensock.core.Animation._tinyNum)
         {
            callback = "onReverseComplete";
            isComplete = this._reversed;
         }
         if(time < 0)
         {
            this._active = false;
            if(this._duration == 0)
            {
               if(this._rawPrevTime >= 0)
               {
                  force = true;
               }
               this._rawPrevTime = rawPrevTime = !(!suppressEvents || time !== 0 || this._rawPrevTime === time) ? com.greensock.core.Animation._tinyNum : time;
            }
         }
         else if(!this._initted)
         {
            force = true;
         }
      }
      else
      {
         this._totalTime = this._time = time;
         if(this._repeat != 0)
         {
            var cycleDuration = this._duration + this._repeatDelay;
            this._cycle = this._totalTime / cycleDuration >> 0;
            switch(this._cycle)
            {
               default:
                  this._cycle--;
                  break;
               case 0:
               case this._totalTime / cycleDuration:
            }
            this._time = this._totalTime - this._cycle * cycleDuration;
            if(this._yoyo)
            {
               if((this._cycle & 1) !== 0)
               {
                  this._time = this._duration - this._time;
               }
            }
            if(this._time > this._duration)
            {
               this._time = this._duration;
            }
            else if(this._time < 0)
            {
               this._time = 0;
            }
         }
         if(this._easeType)
         {
            var r = this._time / this._duration;
            var type = this._easeType;
            var pow = this._easePower;
            if(type === 1 || type === 3 && r >= 0.5)
            {
               r = 1 - r;
            }
            if(type === 3)
            {
               r *= 2;
            }
            if(pow === 1)
            {
               r *= r;
            }
            else if(pow === 2)
            {
               r *= r * r;
            }
            else if(pow === 3)
            {
               r *= r * r * r;
            }
            else if(pow === 4)
            {
               r *= r * r * r * r;
            }
            if(type === 1)
            {
               this.ratio = 1 - r;
            }
            else if(type === 2)
            {
               this.ratio = r;
            }
            else if(this._time / this._duration < 0.5)
            {
               this.ratio = r / 2;
            }
            else
            {
               this.ratio = 1 - r / 2;
            }
         }
         else
         {
            this.ratio = this._ease.getRatio(this._time / this._duration);
         }
      }
      if(prevTime === this._time && !force && this._cycle === prevCycle)
      {
         if(prevTotalTime !== this._totalTime)
         {
            if(this._onUpdate != null)
            {
               if(!suppressEvents)
               {
                  this._onUpdate.apply(this.vars.onUpdateScope || this,this.vars.onUpdateParams);
               }
            }
         }
         return undefined;
      }
      if(!this._initted)
      {
         this._init();
         if(!this._initted || this._gc)
         {
            return undefined;
         }
         if(this._time && !isComplete)
         {
            this.ratio = this._ease.getRatio(this._time / this._duration);
         }
         else if(isComplete && this._ease._calcEnd)
         {
            this.ratio = this._ease.getRatio(this._time !== 0 ? 1 : 0);
         }
      }
      if(!this._active)
      {
         if(!this._paused && this._time !== prevTime && time >= 0)
         {
            this._active = true;
         }
      }
      if(prevTotalTime == 0)
      {
         if(this._startAt != null)
         {
            if(time >= 0)
            {
               this._startAt.render(time,suppressEvents,force);
            }
            else if(!callback)
            {
               callback = "_dummyGS";
            }
         }
         if(this.vars.onStart)
         {
            if(this._totalTime !== 0 || this._duration === 0)
            {
               if(!suppressEvents)
               {
                  this.vars.onStart.apply(this.vars.onStartScope || this,this.vars.onStartParams);
               }
            }
         }
      }
      pt = this._firstPT;
      while(pt)
      {
         if(pt.f)
         {
            pt.t[pt.p](pt.c * this.ratio + pt.s);
         }
         else
         {
            pt.t[pt.p] = pt.c * this.ratio + pt.s;
         }
         pt = pt._next;
      }
      if(this._onUpdate != null)
      {
         if(time < 0 && this._startAt != null && this._startTime != 0)
         {
            this._startAt.render(time,suppressEvents,force);
         }
         if(!suppressEvents)
         {
            if(this._totalTime !== prevTotalTime || isComplete)
            {
               this._onUpdate.apply(this.vars.onUpdateScope || this,this.vars.onUpdateParams);
            }
         }
      }
      if(this._cycle != prevCycle)
      {
         if(!suppressEvents)
         {
            if(!this._gc)
            {
               if(this.vars.onRepeat)
               {
                  this.vars.onRepeat.apply(this.vars.onRepeatScope || this,this.vars.onRepeatParams);
               }
            }
         }
      }
      if(callback)
      {
         if(!this._gc)
         {
            if(time < 0 && this._startAt != null && this._onUpdate == null && this._startTime != 0)
            {
               this._startAt.render(time,suppressEvents,true);
            }
            if(isComplete)
            {
               if(this._timeline.autoRemoveChildren)
               {
                  this._enabled(false,false);
               }
               this._active = false;
            }
            if(!suppressEvents)
            {
               if(this.vars[callback])
               {
                  this.vars[callback].apply(this.vars[callback + "Scope"] || this,this.vars[callback + "Params"]);
               }
            }
            if(this._duration === 0 && this._rawPrevTime === com.greensock.core.Animation._tinyNum && rawPrevTime !== com.greensock.core.Animation._tinyNum)
            {
               this._rawPrevTime = 0;
            }
         }
      }
   }
   static function to(target, duration, vars)
   {
      return new com.greensock.TweenMax(target,duration,vars);
   }
   static function from(target, duration, vars)
   {
      vars.runBackwards = true;
      if(vars.immediateRender != false)
      {
         vars.immediateRender = true;
      }
      return new com.greensock.TweenMax(target,duration,vars);
   }
   static function fromTo(target, duration, fromVars, toVars)
   {
      toVars.startAt = fromVars;
      toVars.immediateRender = toVars.immediateRender != false && fromVars.immediateRender != false;
      return new com.greensock.TweenMax(target,duration,toVars);
   }
   static function staggerTo(targets, duration, vars, stagger, onCompleteAll, onCompleteAllParams, onCompleteAllScope)
   {
      stagger = stagger || 0;
      var a = [];
      var l = targets.length;
      var delay = vars.delay || 0;
      var copy;
      var i;
      var p;
      i = 0;
      while(i < l)
      {
         copy = {};
         for(p in vars)
         {
            copy[p] = vars[p];
         }
         copy.delay = delay;
         if(i == l - 1)
         {
            if(onCompleteAll != null)
            {
               copy.onComplete = function()
               {
                  if(vars.onComplete)
                  {
                     vars.onComplete.apply(vars.onCompleteScope || this,arguments);
                  }
                  onCompleteAll.apply(onCompleteAllScope,onCompleteAllParams);
               };
            }
         }
         a[i] = new com.greensock.TweenMax(targets[i],duration,copy);
         delay += stagger;
         i++;
      }
      return a;
   }
   static function staggerFrom(targets, duration, vars, stagger, onCompleteAll, onCompleteAllParams, onCompleteAllScope)
   {
      vars.runBackwards = true;
      if(vars.immediateRender != false)
      {
         vars.immediateRender = true;
      }
      return com.greensock.TweenMax.staggerTo(targets,duration,vars,stagger,onCompleteAll,onCompleteAllParams,onCompleteAllScope);
   }
   static function staggerFromTo(targets, duration, fromVars, toVars, stagger, onCompleteAll, onCompleteAllParams, onCompleteAllScope)
   {
      toVars.startAt = fromVars;
      toVars.immediateRender = toVars.immediateRender != false && fromVars.immediateRender != false;
      return com.greensock.TweenMax.staggerTo(targets,duration,toVars,stagger,onCompleteAll,onCompleteAllParams,onCompleteAllScope);
   }
   static function delayedCall(delay, callback, params, scope, useFrames)
   {
      return new com.greensock.TweenMax(callback,0,{delay:delay,onComplete:callback,onCompleteParams:params,onCompleteScope:scope,onReverseComplete:callback,onReverseCompleteParams:params,onReverseCompleteScope:scope,immediateRender:false,useFrames:useFrames,overwrite:0});
   }
   static function §set§(target, vars)
   {
      return new com.greensock.TweenMax(target,0,vars);
   }
   static function isTweening(target)
   {
      return com.greensock.TweenLite.getTweensOf(target,true).length > 0;
   }
   static function getAllTweens(includeTimelines)
   {
      var a = com.greensock.TweenMax._getChildrenOf(com.greensock.core.Animation._rootTimeline,includeTimelines);
      return a.concat(com.greensock.TweenMax._getChildrenOf(com.greensock.core.Animation._rootFramesTimeline,includeTimelines));
   }
   static function _getChildrenOf(timeline, includeTimelines)
   {
      if(timeline == null)
      {
         return [];
      }
      var a = [];
      var cnt = 0;
      var tween = timeline._first;
      while(tween)
      {
         if(tween instanceof com.greensock.TweenLite)
         {
            a[cnt++] = tween;
         }
         else
         {
            if(includeTimelines)
            {
               a[cnt++] = tween;
            }
            a = a.concat(com.greensock.TweenMax._getChildrenOf(com.greensock.core.SimpleTimeline(tween),includeTimelines));
            cnt = a.length;
         }
         tween = tween._next;
      }
      return a;
   }
   static function killAll(complete, tweens, delayedCalls, timelines)
   {
      if(tweens == null)
      {
         tweens = true;
      }
      if(delayedCalls == null)
      {
         delayedCalls = true;
      }
      var a = com.greensock.TweenMax.getAllTweens(timelines != false);
      var l = a.length;
      var isDC;
      var allTrue = tweens && delayedCalls && timelines;
      var tween;
      var i;
      i = 0;
      while(i < l)
      {
         tween = a[i];
         if(allTrue || tween instanceof com.greensock.core.SimpleTimeline || (isDC = com.greensock.TweenLite(tween).target == com.greensock.TweenLite(tween).vars.onComplete) && delayedCalls || tweens && !isDC)
         {
            if(complete)
            {
               tween.totalTime(!tween._reversed ? Number(tween.totalDuration()) : 0);
            }
            else
            {
               tween._enabled(false,false);
            }
         }
         i++;
      }
   }
   static function killChildTweensOf(parent, complete)
   {
      var a = com.greensock.TweenMax.getAllTweens(false);
      var l = a.length;
      var i;
      i = 0;
      while(i < l)
      {
         if(com.greensock.TweenMax._containsChildOf(parent,a[i].target))
         {
            if(complete)
            {
               a[i].totalTime(a[i].totalDuration());
            }
            else
            {
               a[i]._enabled(false,false);
            }
         }
         i++;
      }
   }
   static function _containsChildOf(parent, obj)
   {
      var i;
      var curParent;
      if(obj instanceof Array)
      {
         i = obj.length;
         while(--i > -1)
         {
            if(com.greensock.TweenMax._containsChildOf(parent,obj[i]))
            {
               return true;
            }
         }
      }
      else if(typeof obj === "object" && obj._parent instanceof MovieClip)
      {
         curParent = obj._parent;
         while(curParent)
         {
            if(curParent == parent)
            {
               return true;
            }
            curParent = curParent._parent;
         }
      }
      return false;
   }
   static function pauseAll(tweens, delayedCalls, timelines)
   {
      com.greensock.TweenMax._changePause(true,tweens,delayedCalls,timelines);
   }
   static function resumeAll(tweens, delayedCalls, timelines)
   {
      com.greensock.TweenMax._changePause(false,tweens,delayedCalls,timelines);
   }
   static function _changePause(pause, tweens, delayedCalls, timelines)
   {
      if(tweens == undefined)
      {
         tweens = true;
      }
      if(delayedCalls == undefined)
      {
         delayedCalls = true;
      }
      var a = com.greensock.TweenMax.getAllTweens(timelines);
      var isDC;
      var tween;
      var allTrue = tweens && delayedCalls && timelines;
      var i = a.length;
      while(--i > -1)
      {
         tween = a[i];
         if(allTrue || tween instanceof com.greensock.core.SimpleTimeline || (isDC = com.greensock.TweenLite(tween).target == com.greensock.TweenLite(tween).vars.onComplete) && delayedCalls || tweens && !isDC)
         {
            tween.paused(pause);
         }
      }
   }
   function progress(value, suppressEvents)
   {
      return !!arguments.length ? this.totalTime(this.duration() * (!(this._yoyo && (this._cycle & 1) !== 0) ? value : 1 - value) + this._cycle * (this._duration + this._repeatDelay),suppressEvents) : this._time / this.duration();
   }
   function totalProgress(value, suppressEvents)
   {
      return !!arguments.length ? this.totalTime(this.totalDuration() * value,suppressEvents) : this._totalTime / this.totalDuration();
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
      if(this._yoyo && (this._cycle & 1) !== 0)
      {
         value = this._duration - value + this._cycle * (this._duration + this._repeatDelay);
      }
      else if(this._repeat != 0)
      {
         value += this._cycle * (this._duration + this._repeatDelay);
      }
      return this.totalTime(value,suppressEvents);
   }
   function duration(value)
   {
      if(!arguments.length)
      {
         return this._duration;
      }
      return super.duration(value);
   }
   function totalDuration(value)
   {
      if(!arguments.length)
      {
         if(this._dirty)
         {
            this._totalDuration = this._repeat !== -1 ? this._duration * (this._repeat + 1) + this._repeatDelay * this._repeat : 999999999999;
            this._dirty = false;
         }
         return this._totalDuration;
      }
      return this._repeat != -1 ? this.duration((value - this._repeat * this._repeatDelay) / (this._repeat + 1)) : this;
   }
   function repeat(value)
   {
      if(!arguments.length)
      {
         return this._repeat;
      }
      this._repeat = value;
      return this._uncache(true);
   }
   function repeatDelay(value)
   {
      if(!arguments.length)
      {
         return this._repeatDelay;
      }
      this._repeatDelay = value;
      return this._uncache(true);
   }
   function yoyo(value)
   {
      if(!arguments.length)
      {
         return this._yoyo;
      }
      this._yoyo = value;
      return this;
   }
   static function globalTimeScale(value)
   {
      if(!arguments.length)
      {
         return com.greensock.core.Animation._rootTimeline != null ? com.greensock.core.Animation._rootTimeline._timeScale : 1;
      }
      value = value || 0.0001;
      if(com.greensock.core.Animation._rootTimeline == null)
      {
         com.greensock.TweenLite.to({},0,{});
      }
      var tl = com.greensock.core.Animation._rootTimeline;
      var t = getTimer() / 1000;
      tl._startTime = t - (t - tl._startTime) * tl._timeScale / value;
      tl = com.greensock.core.Animation._rootFramesTimeline;
      t = com.greensock.core.Animation._rootFrame;
      tl._startTime = t - (t - tl._startTime) * tl._timeScale / value;
      com.greensock.core.Animation._rootFramesTimeline._timeScale = com.greensock.core.Animation._rootTimeline._timeScale = value;
      return value;
   }
}
