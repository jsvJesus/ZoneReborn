package ui.components
{
   import com.dvalimona.components.*;
   import com.greensock.*;
   import com.greensock.easing.*;
   import flash.display.*;
   import flash.events.*;
   import flash.text.TextField;
   
   public class BoldButton extends PushButton
   {
      protected var _have_background:Boolean = false;
      
      protected var _black_backgound:Sprite;
      
      public function BoldButton(parent:DisplayObjectContainer = null, xpos:Number = 0, ypos:Number = 0, label:String = "", defaultHandler:Function = null)
      {
         super(parent,xpos,ypos,label,defaultHandler);
      }
      
      public function set have_background(value:Boolean) : void
      {
         this._have_background = value;
         this.draw();
      }
      
      public function get have_background() : Boolean
      {
         return this._have_background;
      }
      
      override protected function addChildren() : void
      {
         this._black_backgound = new Sprite();
         addChild(this._black_backgound);
         _face = new noDiagrid();
         addChild(_face);
         _label = new LabelShadowed(this);
         (_label as LabelShadowed).shadowColor = 0;
         (_label as LabelShadowed).shadowAlpha = 0.9;
         (_label as LabelShadowed).shadowSize = 1;
         _label.size = 20;
         _label.font = Base.FONT_BOLD;
         _label.clipContent = true;
         addEventListener(MouseEvent.MOUSE_DOWN,onMouseGoDown);
         addEventListener(MouseEvent.ROLL_OVER,onMouseOver);
         this.autoWidth = true;
         this.upColorAlpha = 0.5;
         this.downColorAlpha = 0.8;
         this.overColorAlpha = 1;
         this.labelUpColor = Style.MENU_LABEL_COLOR;
         this.labelOverColor = Style.MENU_LABEL_OVER_COLOR;
         this.labelDownColor = Style.MENU_LABEL_DOWN_COLOR;
         this.align = Label.LEFT;
         marginLeft = 10;
         marginRight = 10;
      }
      
      override public function draw() : void
      {
         super.draw();
         if(this.have_background)
         {
            this.draw_back();
         }
      }
      
      public function draw_back() : void
      {
         this._black_backgound.graphics.clear();
         this._black_backgound.graphics.beginFill(0,0.4);
         this._black_backgound.graphics.drawRect(0,0,_width,_height);
         this._black_backgound.graphics.endFill();
      }
      
      override protected function drawFace() : void
      {
         if(_over)
         {
            if(_down)
            {
               _label.color = labelDownColor;
               _label.alpha = labelDownColorAlpha;
               TweenMax.killTweensOf(_face);
               TweenMax.to(_face,0.3,{
                  "alpha":downColorAlpha,
                  "ease":Expo.easeOut
               });
            }
            else
            {
               _label.color = labelOverColor;
               _label.alpha = labelOverColorAlpha;
               TweenMax.killTweensOf(_face);
               TweenMax.to(_face,0.3,{
                  "alpha":overColorAlpha,
                  "ease":Expo.easeOut
               });
            }
         }
         else
         {
            _label.color = labelUpColor;
            _label.alpha = enabled ? labelColorAlpha : disabledLabelAlpha;
            TweenMax.killTweensOf(_face);
            TweenMax.to(_face,0.3,{
               "alpha":upColorAlpha,
               "ease":Expo.easeOut
            });
         }
         if(_autoWidth)
         {
            _width = marginLeft + _label.width + marginRight;
         }
         _face.width = _width;
         _face.height = _height;
      }
      
      public function get textField() : TextField
      {
         return _label.textField;
      }
   }
}

