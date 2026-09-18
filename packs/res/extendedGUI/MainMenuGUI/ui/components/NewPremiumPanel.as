package ui.components
{
   import com.dvalimona.components.*;
   import com.greensock.*;
   import com.greensock.easing.*;
   import communication.*;
   import flash.display.*;
   import flash.events.*;
   import flash.utils.*;
   import lang.*;
   import logging.*;
   import ui.*;
   
   public class NewPremiumPanel extends Component
   {
      private var back:ui.components.BlackPanel;
      
      private var caption:LabelShadowed;
      
      private var addPremiumHint:TextShadowed;
      
      private var choicePremium:ClearButton;
      
      private var rejectPremium:ClearButton;
      
      private var showPremium:ClearButton;
      
      private var currentPremiumIcon:Sprite;
      
      private var currentPremiumCaption:LabelShadowed;
      
      private var currentPremiumDescription:TextShadowed;
      
      private var currentPremiumRatio:LabelShadowed;
      
      private var currentPremiumElapsed:LabelShadowed;
      
      private var currentPremiumPS:TextShadowed;
      
      private var currentPremiumBox:VBox;
      
      private var serviceBox:HBox;
      
      private var havePremiumView:Sprite;
      
      private var noPremiumView:Sprite;
      
      private var details:ClearButton;
      
      private var test0:BoldButton;
      
      private var test1:BoldButton;
      
      public function NewPremiumPanel(parent:DisplayObjectContainer = null, xpos:Number = 0, ypos:Number = 0)
      {
         this.width = this.preferredWidth;
         Premium.core.addEventListener(Premium.UPDATED,this.onPremiumUpdated);
         super(parent,xpos,ypos);
      }
      
      private function get preferredWidth() : uint
      {
         return Navigator.LeftWidth;
      }
      
      protected function onPremiumUpdated(event:Event) : void
      {
         Logger.LogToChannel(Logger.DEBUG,"NewPremiumPanel.onPremiumUpdated");
         this.updateView();
      }
      
      override protected function addChildren() : void
      {
         this.back = new BlackPanel();
         this.addChild(this.back);
         this.test0 = new BoldButton();
         this.test0.addEventListener(MouseEvent.CLICK,this.onTest0Click);
         this.test0.label = "no";
         this.test1 = new BoldButton();
         this.test1.label = "0";
         this.test1.addEventListener(MouseEvent.CLICK,this.onTest1Click);
         this.caption = new LabelShadowed(this,0,0,"");
         this.caption.$ = "extendedGUI.RootWindow.premiumButton";
         this.caption.shadowColor = 0;
         this.caption.shadowAlpha = 0.9;
         this.caption.shadowSize = 1;
         this.caption.size = 22;
         this.caption.font = Base.FONT_REGULAR;
         this.caption.color = Style.MENU_LABEL_COLOR;
         this.caption.clipContent = true;
         this.noPremiumView = new Sprite();
         this.addPremiumHint = new TextShadowed(this.noPremiumView);
         this.addPremiumHint.editable = false;
         this.addPremiumHint.selectable = false;
         this.addPremiumHint.autoHeight = true;
         this.addPremiumHint.font = Base.FONT_LIGHT;
         this.addPremiumHint.size = 18;
         this.addPremiumHint.color = 11776934;
         this.addPremiumHint.paddingLeft = 20;
         this.addPremiumHint.paddingRight = 10;
         this.addPremiumHint.paddingBottom = 10;
         this.addPremiumHint.$ = "extendedGUI.RootWindow.premiumHint";
         this.addPremiumHint.debug = false;
         this.addPremiumHint.draw();
         this.havePremiumView = new Sprite();
         this.havePremiumView.addChild(this.currentPremiumIcon = new Sprite());
         this.currentPremiumBox = new VBox(this.havePremiumView);
         this.currentPremiumBox.spacing = 1;
         this.currentPremiumBox.alignment = VBox.LEFT;
         this.currentPremiumBox.debug = false;
         this.currentPremiumCaption = new LabelShadowed(this.currentPremiumBox);
         this.currentPremiumCaption.color = 12700012;
         this.currentPremiumCaption.size = 22;
         this.currentPremiumCaption.font = Base.FONT_REGULAR;
         this.currentPremiumDescription = new TextShadowed(this.currentPremiumBox);
         this.currentPremiumDescription.color = 16777215;
         this.currentPremiumDescription.size = 20;
         this.currentPremiumDescription.selectable = false;
         this.currentPremiumDescription.editable = false;
         this.currentPremiumDescription.font = Base.FONT_LIGHT;
         this.currentPremiumDescription.leading = -5;
         this.currentPremiumDescription.debug = false;
         this.currentPremiumRatio = new LabelShadowed(this.currentPremiumBox);
         this.currentPremiumRatio.color = 12895428;
         this.currentPremiumRatio.size = 17;
         this.currentPremiumRatio.font = Base.FONT_LIGHT;
         this.currentPremiumElapsed = new LabelShadowed(this.currentPremiumBox);
         this.currentPremiumElapsed.color = 12895428;
         this.currentPremiumElapsed.size = 17;
         this.currentPremiumElapsed.font = Base.FONT_LIGHT;
         this.currentPremiumElapsed.paddingTop = -10;
         this.currentPremiumPS = new TextShadowed(this.currentPremiumBox);
         this.currentPremiumPS.color = 12895428;
         this.currentPremiumPS.size = 17;
         this.currentPremiumPS.selectable = false;
         this.currentPremiumPS.editable = false;
         this.currentPremiumPS.font = Base.FONT_LIGHT;
         this.currentPremiumPS.debug = false;
         this.currentPremiumPS.paddingBottom = -20;
         this.currentPremiumPS.leading = -5;
         this.showPremium = new ClearButton(this.havePremiumView);
         this.showPremium.paddingRight = 10;
         this.showPremium.autoWidth = true;
         this.showPremium.align = Label.RIGHT;
         this.showPremium.labelOverColor = 12700012;
         this.showPremium.labelUpColor = 5553663;
         this.showPremium.size = 18;
         this.showPremium.label = Locale.getById("extendedGUI.PremiumPanel.showPremium");
         this.showPremium.addEventListener(MouseEvent.CLICK,this.onShowPremiumClick);
         this.choicePremium = new ClearButton(this.noPremiumView);
         this.choicePremium.paddingRight = 10;
         this.choicePremium.autoWidth = true;
         this.choicePremium.align = Label.RIGHT;
         this.choicePremium.size = 18;
         this.choicePremium.labelOverColor = 12700012;
         this.choicePremium.labelUpColor = 5553663;
         this.choicePremium.label = Locale.getById("extendedGUI.PremiumPanel.choicePremium");
         this.choicePremium.addEventListener(MouseEvent.CLICK,this.onChoicePremiumClick);
         this.rejectPremium = new ClearButton(this.havePremiumView);
         this.rejectPremium.visible = false;
         this.rejectPremium.paddingRight = 10;
         this.rejectPremium.autoWidth = true;
         this.rejectPremium.align = Label.RIGHT;
         this.rejectPremium.size = 18;
         this.rejectPremium.labelOverColor = 12700012;
         this.rejectPremium.labelUpColor = 5553663;
         this.rejectPremium.label = Locale.getById("extendedGUI.PremiumPanel.rejectPremium");
         this.rejectPremium.addEventListener(MouseEvent.CLICK,this.onRejectPremiumClick);
         this.details = new ClearButton();
         this.details.$ = "extendedGUI.PremiumPanel.details";
         this.details.size = 18;
         this.details.height = 32;
         this.details.addEventListener(MouseEvent.CLICK,this.onDetailsClick);
         this.updateView();
         setTimeout(this.updateView,500);
         setTimeout(this.updateView,1000);
         setTimeout(this.updateView,2000);
      }
      
      protected function onDetailsClick(event:MouseEvent) : void
      {
         Base.navigator.showDialog(Premium.Current.caption,Premium.Current.description,false,[new DialogButtonItem("extendedGUI.Dialogs.Ok",null,1)],500,350);
      }
      
      protected function onTest0Click(event:MouseEvent) : void
      {
         Premium.TestNoCurrentPremium();
      }
      
      protected function onTest1Click(event:MouseEvent) : void
      {
         Premium.TestHaveCurrentPremium();
      }
      
      protected function onShowPremiumClick(event:MouseEvent) : void
      {
         var choosePremium:ChoosePremiumWindow = new ChoosePremiumWindow(Base.navigator.premiumDialogs);
      }
      
      protected function onChoicePremiumClick(event:MouseEvent) : void
      {
         var choosePremium:ChoosePremiumWindow = new ChoosePremiumWindow(Base.navigator.premiumDialogs);
      }
      
      protected function onRejectPremiumClick(event:MouseEvent) : void
      {
         Base.navigator.showDialog("extendedGUI.Dialogs.Warning","extendedGUI.Dialogs.askRejectPrem",true,[new DialogButtonItem("extendedGUI.Dialogs.Ok",this.onRejectOkButton,0.61),new DialogButtonItem("extendedGUI.Dialogs.Cancel",null,0.39)]);
      }
      
      protected function onRejectOkButton() : *
      {
         this.rejectPremium.visible = false;
         Api.call(Api.REJECT_PREM);
      }
      
      protected function get havePremiums() : Boolean
      {
         return Premium.Current != null;
      }
      
      private function getTimeToEnd(time:Number) : String
      {
         var days:uint = 0;
         var real_days:uint = 0;
         var hours:uint = 0;
         var minutes:uint = 0;
         var result:String = "";
         var timeToRemain:uint = time;
         real_days = timeToRemain / (60 * 60 * 24);
         days = Math.ceil(timeToRemain / (60 * 60 * 24));
         hours = timeToRemain / (60 * 60);
         minutes = timeToRemain / 60;
         if(real_days >= 1)
         {
            result = Locale.getById("extendedGUI.PremiumPanel.days") + " " + Locale.getById("extendedGUI.PremiumPanel.toEnd") + ": " + days;
         }
         else if(hours >= 1)
         {
            result = Locale.getById("extendedGUI.PremiumPanel.hours") + " " + Locale.getById("extendedGUI.PremiumPanel.toEnd") + ": " + hours;
         }
         else
         {
            result = Locale.getById("extendedGUI.PremiumPanel.minutes") + " " + Locale.getById("extendedGUI.PremiumPanel.toEnd") + ": " + minutes;
         }
         return result;
      }
      
      public function updateView() : void
      {
         var i:* = undefined;
         Logger.LogToChannel(Logger.DEBUG,"NewPremiumPanel.updateView",this.havePremiums);
         if(this.havePremiums)
         {
            if(this.contains(this.noPremiumView))
            {
               this.removeChild(this.noPremiumView);
            }
            if(!this.contains(this.havePremiumView))
            {
               this.addChild(this.havePremiumView);
            }
            this.currentPremiumBox.x = 160;
            this.currentPremiumBox.y = 40;
            this.currentPremiumCaption.text = Premium.Current.caption;
            if(Premium.Current.description.length < Premium.MAX_DESCRIPTION_LENGTH)
            {
               this.currentPremiumDescription.text = Premium.Current.description;
               if(this.contains(this.details))
               {
                  this.removeChild(this.details);
               }
            }
            else
            {
               this.currentPremiumDescription.text = Premium.Current.description.substr(0,Premium.MAX_DESCRIPTION_LENGTH) + "…";
               if(!this.contains(this.details))
               {
                  this.addChild(this.details);
               }
            }
            this.currentPremiumRatio.text = Locale.getById("extendedGUI.PremiumPanel.expCoefficient") + " " + Premium.Current.ratio;
            this.currentPremiumElapsed.text = this.getTimeToEnd(Premium.Elapsed);
            if(Premium.Current.holiday)
            {
               this.rejectPremium.visible = true;
               this.currentPremiumPS.text = Locale.getById("extendedGUI.PremiumPanel.freezeText");
               if(Premium.frizeID != -1)
               {
                  this.currentPremiumPS.text += Locale.getById("extendedGUI.PremiumPanel.freezeText1");
               }
            }
            else
            {
               this.rejectPremium.visible = false;
               this.currentPremiumPS.text = "";
            }
            this.currentPremiumDescription.width = this.preferredWidth - this.currentPremiumBox.x - 10 - 0;
            this.currentPremiumPS.width = this.preferredWidth - this.currentPremiumBox.x - 10 - 0;
            for(i = this.currentPremiumIcon.numChildren - 1; i >= 0; i--)
            {
               this.currentPremiumIcon.removeChildAt(i);
            }
            this.currentPremiumIcon.addChild(Premium.Current.icon);
            setTimeout(this.tuneHeight,100,false);
            setTimeout(this.tuneHeight,400,false);
         }
         else
         {
            if(!this.contains(this.noPremiumView))
            {
               this.addChild(this.noPremiumView);
            }
            if(this.contains(this.havePremiumView))
            {
               this.removeChild(this.havePremiumView);
            }
            this.addPremiumHint.width = this.preferredWidth - 30 - this.choicePremium.width;
            setTimeout(this.tuneHeight,100,true);
            setTimeout(this.tuneHeight,400,true);
         }
         invalidate();
      }
      
      protected function tuneHeight(noPremiumView:Boolean) : void
      {
         var th:Number = NaN;
         if(noPremiumView)
         {
            TweenMax.to(this,0.3,{
               "height":this.addPremiumHint.y + this.addPremiumHint.height + 10,
               "width":this.preferredWidth,
               "ease":Expo.easeOut
            });
         }
         else
         {
            th = Math.max(this.currentPremiumBox.y + this.currentPremiumElapsed.y + this.currentPremiumElapsed.height + 20,this.currentPremiumIcon.y + 150 + 10);
            TweenMax.to(this,0.3,{
               "height":th,
               "width":this.preferredWidth,
               "ease":Expo.easeOut
            });
         }
      }
      
      override public function draw() : void
      {
         super.draw();
         this.test0.x = -this.test0.width - 10;
         this.test1.x = -this.test1.width - 10;
         this.test1.y = this.test0.y + this.test0.height + 1;
         this.back.width = width;
         this.back.height = height;
         this.caption.x = 10;
         this.caption.y = 10;
         this.addPremiumHint.x = 20;
         this.addPremiumHint.y = 40;
         this.currentPremiumIcon.y = this.caption.y + this.caption.height + 1;
         this.addPremiumHint.width = this.width - 20;
         this.choicePremium.x = this.width - this.choicePremium.width - 10;
         this.showPremium.x = this.width - this.showPremium.width - 10;
         this.rejectPremium.x = this.showPremium.x - this.rejectPremium.width - 50;
         this.choicePremium.y = this.showPremium.y = 16;
         this.rejectPremium.y = this.showPremium.y;
         this.details.x = this.width - this.details.width;
         this.details.y = this.height - this.details.height - 3;
      }
   }
}

