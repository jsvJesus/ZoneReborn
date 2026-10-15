class com.greensock.TweenLite extends com.greensock.core.Animation
{
   var ratio;
   var target;
   var _ease;
   var _overwrite;
   var vars;
   var _targets;
   var _propLookup;
   var _siblings;
   var _delay;
   var _startAt;
   var _time;
   var _duration;
   var _easeType;
   var _easePower;
   var _firstPT;
   var _overwrittenProps;
   var _onUpdate;
   var _initted;
   var _notifyPluginsOfEnabled;
   var _totalTime;
   var _reversed;
   var _rawPrevTime;
   var _startTime;
   var _timeline;
   var _active;
   var _gc;
   var _paused;
   static var _overwriteLookup;
   static var _onPluginEvent;
   static var version = "12.1.5";
   static var defaultEase = new com.greensock.easing.Ease(null,null,1,1);
   static var defaultOverwrite = "auto";
   static var ticker = com.greensock.core.Animation.ticker;
   static var _plugins = {};
   static var _tweenLookup = {};
   static var _cnt = 0;
   static var _reservedProps = {ease:1,delay:1,overwrite:1,onComplete:1,onCompleteParams:1,onCompleteScope:1,useFrames:1,runBackwards:1,startAt:1,onUpdate:1,onUpdateParams:1,onUpdateScope:1,onStart:1,onStartParams:1,onStartScope:1,onReverseComplete:1,onReverseCompleteParams:1,onReverseCompleteScope:1,onRepeat:1,onRepeatParams:1,onRepeatScope:1,easeParams:1,yoyo:1,orientToBezier:1,immediateRender:1,repeat:1,repeatDelay:1,data:1,paused:1,reversed:1};
   function TweenLite(target, duration, vars)
   {
      super(duration,vars);
      if(!com.greensock.TweenLite._overwriteLookup)
      {
         com.greensock.TweenLite._overwriteLookup = {none:0,all:1,auto:2,concurrent:3,allOnStart:4,preexisting:5};
         com.greensock.TweenLite._overwriteLookup["true"] = 1;
         com.greensock.TweenLite._overwriteLookup["false"] = 0;
         com.greensock.core.Animation._addTickListener("tick",com.greensock.TweenLite._dumpGarbage,com.greensock.TweenLite);
      }
      this.ratio = 0;
      this.target = target;
      this._ease = com.greensock.TweenLite.defaultEase;
      this._overwrite = this.vars.overwrite != null ? (typeof this.vars.overwrite !== "number" ? com.greensock.TweenLite._overwriteLookup[this.vars.overwrite] : this.vars.overwrite >> 0) : com.greensock.TweenLite._overwriteLookup[com.greensock.TweenLite.defaultOverwrite];
      if(this.target instanceof Array && (typeof this.target[0] === "object" || typeof this.target[0] === "movieclip"))
      {
         this._targets = this.target.concat();
         this._propLookup = [];
         this._siblings = [];
         var i = this._targets.length;
         while(--i > -1)
         {
            this._siblings[i] = com.greensock.TweenLite._register(this._targets[i],this,false);
            if(this._overwrite === 1)
            {
               if(this._siblings[i].length > 1)
               {
                  com.greensock.TweenLite._applyOverwrite(this._targets[i],this,null,1,this._siblings[i]);
               }
            }
         }
      }
      else
      {
         this._propLookup = {};
         this._siblings = com.greensock.TweenLite._register(target,this,false);
         if(this._overwrite === 1)
         {
            if(this._siblings.length > 1)
            {
               com.greensock.TweenLite._applyOverwrite(target,this,null,1,this._siblings);
            }
         }
      }
      if(this.vars.immediateRender || duration === 0 && this._delay === 0 && this.vars.immediateRender != false)
      {
         this.render(- this._delay,false,true);
      }
   }
   function _init()
   {
      var immediate = this.vars.immediateRender;
      var i;
      var initPlugins;
      var pt;
      var p;
      var copy;
      if(this.vars.startAt)
      {
         if(this._startAt != null)
         {
            this._startAt.render(-1,true);
         }
         this.vars.startAt.overwrite = 0;
         this.vars.startAt.immediateRender = true;
         this._startAt = new com.greensock.TweenLite(this.target,0,this.vars.startAt);
         if(immediate)
         {
            if(this._time > 0)
            {
               this._startAt = null;
            }
            else if(this._duration !== 0)
            {
               return undefined;
            }
         }
      }
      else if(this.vars.runBackwards && this._duration !== 0)
      {
         if(this._startAt != null)
         {
            this._startAt.render(-1,true);
            this._startAt = null;
         }
         else
         {
            copy = {};
            for(p in this.vars)
            {
               if(com.greensock.TweenLite._reservedProps[p] == null)
               {
                  copy[p] = this.vars[p];
               }
            }
            copy.overwrite = 0;
            copy.data = "isFromStart";
            this._startAt = com.greensock.TweenLite.to(this.target,0,copy);
            if(!immediate)
            {
               this._startAt.render(-1,true);
            }
            else if(this._time === 0)
            {
               return undefined;
            }
         }
      }
      if(this.vars.ease instanceof com.greensock.easing.Ease)
      {
         this._ease = !(this.vars.easeParams instanceof Array) ? this.vars.ease : this.vars.ease.config.apply(this.vars.ease,this.vars.easeParams);
      }
      else if(typeof this.vars.ease === "function")
      {
         this._ease = new com.greensock.easing.Ease(this.vars.ease,this.vars.easeParams);
      }
      else
      {
         this._ease = com.greensock.TweenLite.defaultEase;
      }
      this._easeType = this._ease._type;
      this._easePower = this._ease._power;
      this._firstPT = null;
      if(this._targets)
      {
         i = this._targets.length;
         while(--i > -1)
         {
            if(this._initProps(this._targets[i],this._propLookup[i] = {},this._siblings[i],!this._overwrittenProps ? null : this._overwrittenProps[i]))
            {
               initPlugins = true;
            }
         }
      }
      else
      {
         initPlugins = this._initProps(this.target,this._propLookup,this._siblings,this._overwrittenProps);
      }
      if(initPlugins)
      {
         com.greensock.TweenLite._onPluginEvent("_onInitAllProps",this);
      }
      if(this._overwrittenProps)
      {
         if(this._firstPT == null)
         {
            if(typeof this.target !== "function")
            {
               this._enabled(false,false);
            }
         }
      }
      if(this.vars.runBackwards)
      {
         pt = this._firstPT;
         while(pt)
         {
            pt.s += pt.c;
            pt.c = - pt.c;
            pt = pt._next;
         }
      }
      this._onUpdate = this.vars.onUpdate;
      this._initted = true;
   }
   function _initProps(target, propLookup, siblings, overwrittenProps)
   {
      var p;
      var i;
      var initPlugins;
      var plugin;
      var val;
      if(target == null)
      {
         return false;
      }
      for(p in this.vars)
      {
         val = this.vars[p];
         if(com.greensock.TweenLite._reservedProps[p])
         {
            if(val instanceof Array)
            {
               if(val.join("").indexOf("{self}") !== -1)
               {
                  this.vars[p] = this._swapSelfInParams(val);
               }
            }
         }
         else if(com.greensock.TweenLite._plugins[p] && (plugin = new com.greensock.TweenLite._plugins[p]())._onInitTween(target,this.vars[p],this))
         {
            this._firstPT = {_next:this._firstPT,t:plugin,p:"setRatio",s:0,c:1,f:true,n:p,pg:true,pr:plugin._priority};
            i = plugin._overwriteProps.length;
            while(--i > -1)
            {
               propLookup[plugin._overwriteProps[i]] = this._firstPT;
            }
            if(plugin._priority || plugin._onInitAllProps)
            {
               initPlugins = true;
            }
            if(plugin._onDisable || plugin._onEnable)
            {
               this._notifyPluginsOfEnabled = true;
            }
         }
         else
         {
            this._firstPT = propLookup[p] = {_next:this._firstPT,t:target,p:p,f:typeof target[p] === "function",n:p,pg:false,pr:0};
            this._firstPT.s = !!this._firstPT.f ? target[!(p.indexOf("set") || typeof target["get" + p.substr(3)] !== "function") ? "get" + p.substr(3) : p]() : Number(target[p]);
            this._firstPT.c = typeof val !== "number" ? (!(typeof val === "string" && val.charAt(1) === "=") ? Number(val) || 0 : Number(val.charAt(0) + "1") * Number(val.substr(2))) : Number(val) - this._firstPT.s;
         }
         if(this._firstPT)
         {
            if(this._firstPT._next)
            {
               this._firstPT._next._prev = this._firstPT;
            }
         }
      }
      if(overwrittenProps)
      {
         if(this._kill(overwrittenProps,target))
         {
            return this._initProps(target,propLookup,siblings,overwrittenProps);
         }
      }
      if(this._overwrite > 1)
      {
         if(this._firstPT)
         {
            if(siblings.length > 1)
            {
               if(com.greensock.TweenLite._applyOverwrite(target,this,propLookup,this._overwrite,siblings))
               {
                  this._kill(propLookup,target);
                  return this._initProps(target,propLookup,siblings,overwrittenProps);
               }
            }
         }
      }
      return initPlugins;
   }
   function render(time, suppressEvents, force)
   {
      var isComplete;
      var callback;
      var pt;
      var rawPrevTime;
      var prevTime = this._time;
      if(time >= this._duration)
      {
         this._totalTime = this._time = this._duration;
         this.ratio = !this._ease._calcEnd ? 1 : this._ease.getRatio(1);
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
            this._rawPrevTime = rawPrevTime = !(!suppressEvents || time !== 0 || rawPrevTime === time) ? com.greensock.core.Animation._tinyNum : time;
         }
      }
      else if(time < 1e-7)
      {
         this._totalTime = this._time = 0;
         this.ratio = !this._ease._calcEnd ? 0 : this._ease.getRatio(0);
         if(prevTime !== 0 || this._duration === 0 && this._rawPrevTime > 0 && this._rawPrevTime !== com.greensock.core.Animation._tinyNum)
         {
            callback = "onReverseComplete";
            isComplete = this._reversed;
         }
         if(time < 0)
         {
            this._active = false;
            if(this._duration === 0)
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
         if(this._easeType)
         {
            var r = time / this._duration;
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
            else if(time / this._duration < 0.5)
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
            this.ratio = this._ease.getRatio(time / this._duration);
         }
      }
      if(this._time === prevTime && !force)
      {
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
      if(prevTime === 0)
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
            if(this._time !== 0 || this._duration === 0)
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
            if(this._time !== prevTime || isComplete)
            {
               this._onUpdate.apply(this.vars.onUpdateScope || this,this.vars.onUpdateParams);
            }
         }
      }
      if(callback)
      {
         if(!this._gc)
         {
            if(time < 0 && this._startAt != null && this._onUpdate == null && this._startTime != 0)
            {
               this._startAt.render(time,suppressEvents,force);
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
   function _kill(vars, target)
   {
      if(vars === "all")
      {
         vars = null;
      }
      if(vars == null)
      {
         if(target == null || target == this.target)
         {
            return this._enabled(false,false);
         }
      }
      target = target || this._targets || this.target;
      var i;
      var overwrittenProps;
      var p;
      var pt;
      var propLookup;
      var changed;
      var killProps;
      var record;
      if(target instanceof Array && (typeof target[0] === "object" || typeof target[0] === "movieclip"))
      {
         i = target.length;
         while(--i > -1)
         {
            if(this._kill(vars,target[i]))
            {
               changed = true;
            }
         }
      }
      else
      {
         if(this._targets)
         {
            i = this._targets.length;
            while(--i > -1)
            {
               if(target === this._targets[i])
               {
                  propLookup = this._propLookup[i] || {};
                  this._overwrittenProps = this._overwrittenProps || [];
                  overwrittenProps = this._overwrittenProps[i] = !vars ? "all" : this._overwrittenProps[i] || {};
                  break;
               }
            }
         }
         else
         {
            if(target !== this.target)
            {
               return false;
            }
            propLookup = this._propLookup;
            overwrittenProps = this._overwrittenProps = !vars ? "all" : this._overwrittenProps || {};
         }
         if(propLookup)
         {
            killProps = vars || propLookup;
            record = vars != overwrittenProps && overwrittenProps != "all" && vars != propLookup && (typeof vars != "object" || vars._tempKill != true);
            for(p in killProps)
            {
               if(pt = propLookup[p])
               {
                  if(pt.pg && pt.t._kill(killProps))
                  {
                     changed = true;
                  }
                  if(!pt.pg || pt.t._overwriteProps.length === 0)
                  {
                     if(pt._prev)
                     {
                        pt._prev._next = pt._next;
                     }
                     else if(pt == this._firstPT)
                     {
                        this._firstPT = pt._next;
                     }
                     if(pt._next)
                     {
                        pt._next._prev = pt._prev;
                     }
                     pt._next = pt._prev = null;
                  }
                  delete propLookup[p];
               }
               if(record)
               {
                  overwrittenProps[p] = 1;
               }
            }
            if(this._firstPT == null && this._initted)
            {
               this._enabled(false,false);
            }
         }
      }
      return changed;
   }
   function invalidate()
   {
      if(this._notifyPluginsOfEnabled)
      {
         com.greensock.TweenLite._onPluginEvent("_onDisable",this);
      }
      this._firstPT = null;
      this._overwrittenProps = null;
      this._onUpdate = null;
      this._startAt = null;
      this._initted = this._active = this._notifyPluginsOfEnabled = false;
      this._propLookup = !this._targets ? [] : {};
      return this;
   }
   function _enabled(enabled, ignoreTimeline)
   {
      if(enabled && this._gc)
      {
         if(this._targets)
         {
            var i = this._targets.length;
            while(--i > -1)
            {
               this._siblings[i] = com.greensock.TweenLite._register(this._targets[i],this,true);
            }
         }
         else
         {
            this._siblings = com.greensock.TweenLite._register(this.target,this,true);
         }
      }
      super._enabled(enabled,ignoreTimeline);
      if(this._notifyPluginsOfEnabled)
      {
         if(this._firstPT)
         {
            return com.greensock.TweenLite._onPluginEvent(!enabled ? "_onDisable" : "_onEnable",this);
         }
      }
      return false;
   }
   static function to(target, duration, vars)
   {
      return new com.greensock.TweenLite(target,duration,vars);
   }
   static function from(target, duration, vars)
   {
      vars.runBackwards = true;
      if(vars.immediateRender != false)
      {
         vars.immediateRender = true;
      }
      return new com.greensock.TweenLite(target,duration,vars);
   }
   static function fromTo(target, duration, fromVars, toVars)
   {
      toVars.startAt = fromVars;
      toVars.immediateRender = toVars.immediateRender != false && fromVars.immediateRender != false;
      return new com.greensock.TweenLite(target,duration,toVars);
   }
   static function delayedCall(delay, callback, params, scope, useFrames)
   {
      return new com.greensock.TweenLite(callback,0,{delay:delay,onComplete:callback,onCompleteParams:params,onCompleteScope:scope,onReverseComplete:callback,onReverseCompleteParams:params,onReverseCompleteScope:scope,immediateRender:false,useFrames:useFrames,overwrite:0});
   }
   static function _dumpGarbage()
   {
      if(!(com.greensock.core.Animation._rootFrame % 60))
      {
         var i;
         var a;
         var p;
         for(p in com.greensock.TweenLite._tweenLookup)
         {
            a = com.greensock.TweenLite._tweenLookup[p].tweens;
            i = a.length;
            while(--i > -1)
            {
               if(a[i]._gc)
               {
                  a.splice(i,1);
               }
            }
            if(a.length === 0)
            {
               delete com.greensock.TweenLite._tweenLookup[p];
            }
         }
      }
   }
   static function §set§(target, vars)
   {
      return new com.greensock.TweenLite(target,0,vars);
   }
   static function killTweensOf(target, onlyActive, vars)
   {
      if(typeof onlyActive === "object")
      {
         vars = onlyActive;
         onlyActive = false;
      }
      var a = com.greensock.TweenLite.getTweensOf(target,onlyActive);
      var i = a.length;
      while(--i > -1)
      {
         a[i]._kill(vars,target);
      }
   }
   static function killDelayedCallsTo(func)
   {
      com.greensock.TweenLite.killTweensOf(func);
   }
   static function getTweensOf(target, onlyActive)
   {
      var i;
      var a;
      var j;
      var t;
      if(target instanceof Array && (typeof target[0] === "object" || typeof target[0] === "movieclip"))
      {
         i = target.length;
         a = [];
         while(--i > -1)
         {
            a = a.concat(com.greensock.TweenLite.getTweensOf(target[i],onlyActive));
         }
         i = a.length;
         while(--i > -1)
         {
            t = a[i];
            j = i;
            while(--j > -1)
            {
               if(t === a[j])
               {
                  a.splice(i,1);
               }
            }
         }
      }
      else
      {
         a = com.greensock.TweenLite._register(target).concat();
         i = a.length;
         while(--i > -1)
         {
            if(a[i]._gc || onlyActive && !a[i].isActive())
            {
               a.splice(i,1);
            }
         }
      }
      return a;
   }
   static function _register(target, tween, scrub)
   {
      var id;
      var i;
      var a;
      var p;
      var tl = com.greensock.TweenLite._tweenLookup;
      if(typeof target === "movieclip")
      {
         id = String(target);
      }
      else
      {
         for(p in tl)
         {
            if(tl[p].target === target)
            {
               id = p;
               break;
            }
         }
      }
      if(!tl[id || (id = "t" + com.greensock.TweenLite._cnt++)])
      {
         tl[id] = {target:target,tweens:[]};
      }
      if(tween)
      {
         a = tl[id].tweens;
         a[i = a.length] = tween;
         if(scrub)
         {
            while(--i > -1)
            {
               if(a[i] === tween)
               {
                  a.splice(i,1);
               }
            }
         }
      }
      return tl[id].tweens;
   }
   static function _applyOverwrite(target, tween, props, mode, siblings)
   {
      var i;
      var changed;
      var curTween;
      if(mode === 1 || mode >= 4)
      {
         var l = siblings.length;
         i = 0;
         while(i < l)
         {
            if((curTween = siblings[i]) !== tween)
            {
               if(!curTween._gc)
               {
                  if(curTween._enabled(false,false))
                  {
                     changed = true;
                  }
               }
            }
            else if(mode === 5)
            {
               break;
            }
            i++;
         }
         return changed;
      }
      var startTime = tween._startTime + 1e-10;
      var overlaps = [];
      var oCount = 0;
      var zeroDur = tween._duration == 0;
      var globalStart;
      i = siblings.length;
      while(--i > -1)
      {
         if(!((curTween = siblings[i]) === tween || curTween._gc || curTween._paused))
         {
            if(curTween._timeline != tween._timeline)
            {
               globalStart = globalStart || com.greensock.TweenLite._checkOverlap(tween,0,zeroDur);
               if(com.greensock.TweenLite._checkOverlap(curTween,globalStart,zeroDur) === 0)
               {
                  overlaps[oCount++] = curTween;
               }
            }
            else if(curTween._startTime <= startTime)
            {
               if(curTween._startTime + curTween.totalDuration() / curTween._timeScale > startTime)
               {
                  if(!((zeroDur || !curTween._initted) && startTime - curTween._startTime <= 2e-10))
                  {
                     overlaps[oCount++] = curTween;
                  }
               }
            }
         }
      }
      i = oCount;
      while(--i > -1)
      {
         curTween = overlaps[i];
         if(mode === 2)
         {
            if(curTween._kill(props,target))
            {
               changed = true;
            }
         }
         if(mode !== 2 || !curTween._firstPT && curTween._initted)
         {
            if(curTween._enabled(false,false))
            {
               changed = true;
            }
         }
      }
      return changed;
   }
   static function _checkOverlap(tween, reference, zeroDur)
   {
      var tl = tween._timeline;
      var ts = tl._timeScale;
      var t = tween._startTime;
      var min = 1e-10;
      while(tl._timeline)
      {
         t += tl._startTime;
         ts *= tl._timeScale;
         if(tl._paused)
         {
            return -100;
         }
         tl = tl._timeline;
      }
      t /= ts;
      return t <= reference ? (!(zeroDur && t == reference || !tween._initted && t - reference < 2 * min) ? ((t += tween.totalDuration() / tween._timeScale / ts) <= reference + min ? t - reference - min : 0) : min) : t - reference;
   }
}
