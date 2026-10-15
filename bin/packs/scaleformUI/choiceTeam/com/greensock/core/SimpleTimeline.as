class com.greensock.core.SimpleTimeline extends com.greensock.core.Animation
{
   var autoRemoveChildren;
   var smoothChildTiming;
   var _last;
   var _sortChildren;
   var _first;
   var _timeline;
   var _totalTime;
   var _time;
   var _rawPrevTime;
   function SimpleTimeline(vars)
   {
      super(0,vars);
      this.autoRemoveChildren = this.smoothChildTiming = true;
   }
   function insert(tween, time)
   {
      return this.add(tween,time || 0);
   }
   function add(child, position, align, stagger)
   {
      child._startTime = Number(position || 0) + child._delay;
      if(child._paused)
      {
         if(this != child._timeline)
         {
            child._pauseTime = child._startTime + (this.rawTime() - child._startTime) / child._timeScale;
         }
      }
      if(child.timeline)
      {
         child.timeline._remove(child,true);
      }
      child.timeline = child._timeline = this;
      if(child._gc)
      {
         child._enabled(true,true);
      }
      var prevTween = this._last;
      if(this._sortChildren)
      {
         var st = child._startTime;
         while(prevTween && prevTween._startTime > st)
         {
            prevTween = prevTween._prev;
         }
      }
      if(prevTween)
      {
         child._next = prevTween._next;
         prevTween._next = com.greensock.core.Animation(child);
      }
      else
      {
         child._next = this._first;
         this._first = com.greensock.core.Animation(child);
      }
      if(child._next)
      {
         child._next._prev = child;
      }
      else
      {
         this._last = com.greensock.core.Animation(child);
      }
      child._prev = prevTween;
      if(this._timeline)
      {
         this._uncache(true);
      }
      return this;
   }
   function _remove(tween, skipDisable)
   {
      if(tween.timeline == this)
      {
         if(!skipDisable)
         {
            tween._enabled(false,true);
         }
         if(tween._prev)
         {
            tween._prev._next = tween._next;
         }
         else if(this._first === tween)
         {
            this._first = tween._next;
         }
         if(tween._next)
         {
            tween._next._prev = tween._prev;
         }
         else if(this._last === tween)
         {
            this._last = tween._prev;
         }
         tween._next = tween._prev = tween.timeline = null;
         if(this._timeline)
         {
            this._uncache(true);
         }
      }
      return this;
   }
   function render(time, suppressEvents, force)
   {
      var tween = this._first;
      var next;
      this._totalTime = this._time = this._rawPrevTime = time;
      while(tween)
      {
         next = tween._next;
         if(tween._active || time >= tween._startTime && !tween._paused)
         {
            if(!tween._reversed)
            {
               tween.render((time - tween._startTime) * tween._timeScale,suppressEvents,force);
            }
            else
            {
               tween.render((!!tween._dirty ? tween.totalDuration() : tween._totalDuration) - (time - tween._startTime) * tween._timeScale,suppressEvents,force);
            }
         }
         tween = next;
      }
   }
   function rawTime()
   {
      return this._totalTime;
   }
}
