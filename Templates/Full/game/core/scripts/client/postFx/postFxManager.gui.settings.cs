//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

$PostFXManager::defaultPreset  = "core/scripts/client/postFx/default.postfxpreset.cs";

function PostFXManager::settingsSetEnabled(%this, %bEnablePostFX)
{
   $PostFXManager::PostFX::Enabled = %bEnablePostFX;
   
   //if to enable the postFX, apply the ones that are enabled
   if ( %bEnablePostFX )
   {
      if ( $PostFXManager::PostFX::EnableHDR )
         HDRPostFX.enable();
      else
         HDRPostFX.disable();

      postVerbose("% - PostFX Manager - PostFX enabled");      
   }
   else
   {
      //Disable all postFX
      
      HDRPostFX.disable();
      
      postVerbose("% - PostFX Manager - PostFX disabled");
   }
}

function PostFXManager::settingsEffectSetEnabled(%this, %sName, %bEnable)
{
   %postEffect = 0;
   
   //Determine the postFX to enable, and apply the boolean
   if(%sName $= "HDR")
   {
      %postEffect = HDRPostFX;
      $PostFXManager::PostFX::EnableHDR = %bEnable;
      //$pref::PostFX::HDR::Enabled = %bEnable;
   }
   
   // Apply the change
   if ( %bEnable == true )
   {
      %postEffect.enable();
      postVerbose("% - PostFX Manager - " @ %sName @ " enabled");
   }
   else
   {
      %postEffect.disable();
      postVerbose("% - PostFX Manager - " @ %sName @ " disabled");
   }
}

function PostFXManager::settingsRefreshHDR(%this)
{
  //Apply the enabled flag 
   ppOptionsEnableHDR.setValue($PostFXManager::PostFX::EnableHDR);   
    
   ppOptionsHDRBloom.setValue($HDRPostFX::enableBloom);
   ppOptionsHDRBloomBlurBrightPassThreshold.setValue($HDRPostFX::brightPassThreshold);
   ppOptionsHDRBloomBlurMean.setValue($HDRPostFX::gaussMean);
   ppOptionsHDRBloomBlurMultiplier.setValue($HDRPostFX::gaussMultiplier);
   ppOptionsHDRBloomBlurStdDev.setValue($HDRPostFX::gaussStdDev);
   ppOptionsHDRBrightnessAdaptRate.setValue($HDRPostFX::adaptRate);
   ppOptionsHDREffectsBlueShift.setValue($HDRPostFX::enableBlueShift);
   ppOptionsHDREffectsBlueShiftColor.BaseColor = $HDRPostFX::blueShiftColor;
   ppOptionsHDREffectsBlueShiftColor.PickColor = $HDRPostFX::blueShiftColor;
   ppOptionsHDRKeyValue.setValue($HDRPostFX::keyValue);
   ppOptionsHDRMinLuminance.setValue($HDRPostFX::minLuminace);
   ppOptionsHDRToneMapping.setValue($HDRPostFX::enableToneMapping);
   ppOptionsHDRToneMappingAmount.setValue($HDRPostFX::enableToneMapping);
   ppOptionsHDRWhiteCutoff.setValue($HDRPostFX::whiteCutoff);
   
   %this-->ColorCorrectionFileName.Text = $HDRPostFX::colorCorrectionRamp;    
}

function PostFXManager::settingsRefreshAll(%this)
{    
   $PostFXManager::PostFX::Enabled           = $pref::enablePostEffects;
   $PostFXManager::PostFX::EnableHDR         = HDRPostFX.isEnabled();
   
   //For all the postFX here, apply the active settings in the system
   //to the gui controls.
   
   %this.settingsRefreshHDR();
   
   ppOptionsEnable.setValue($PostFXManager::PostFX::Enabled);

   postVerbose("% - PostFX Manager - GUI values updated.");
}

function PostFXManager::settingsApplyFromPreset(%this)
{
   postVerbose("% - PostFX Manager - Applying from preset");

   
   //HDR settings
   $HDRPostFX::adaptRate               = $PostFXManager::Settings::HDR::adaptRate;
   $HDRPostFX::blueShiftColor          = $PostFXManager::Settings::HDR::blueShiftColor;
   $HDRPostFX::brightPassThreshold     = $PostFXManager::Settings::HDR::brightPassThreshold; 
   $HDRPostFX::enableBloom             = $PostFXManager::Settings::HDR::enableBloom;
   $HDRPostFX::enableBlueShift         = $PostFXManager::Settings::HDR::enableBlueShift;
   $HDRPostFX::enableToneMapping       = $PostFXManager::Settings::HDR::enableToneMapping;
   $HDRPostFX::gaussMean               = $PostFXManager::Settings::HDR::gaussMean;
   $HDRPostFX::gaussMultiplier         = $PostFXManager::Settings::HDR::gaussMultiplier;
   $HDRPostFX::gaussStdDev             = $PostFXManager::Settings::HDR::gaussStdDev;
   $HDRPostFX::keyValue                = $PostFXManager::Settings::HDR::keyValue;
   $HDRPostFX::minLuminace             = $PostFXManager::Settings::HDR::minLuminace;
   $HDRPostFX::whiteCutoff             = $PostFXManager::Settings::HDR::whiteCutoff;
   $HDRPostFX::colorCorrectionRamp     = $PostFXManager::Settings::ColorCorrectionRamp;
   
   
  
   if ( $PostFXManager::forceEnableFromPresets )
   {
      $PostFXManager::PostFX::Enabled           = $PostFXManager::Settings::EnablePostFX;
      $PostFXManager::PostFX::EnableHDR         = $PostFXManager::Settings::EnableHDR;

      %this.settingsSetEnabled( true );
   }
   
   
   // Update the actual GUI controls if its awake ( otherwise it will when opened ).
   if ( PostFXManager.isAwake() )
      %this.settingsRefreshAll();      
}

function PostFXManager::settingsApplyHDR(%this)
{   
   $PostFXManager::Settings::HDR::adaptRate                 = $HDRPostFX::adaptRate;
   $PostFXManager::Settings::HDR::blueShiftColor            = $HDRPostFX::blueShiftColor;
   $PostFXManager::Settings::HDR::brightPassThreshold       = $HDRPostFX::brightPassThreshold;
   $PostFXManager::Settings::HDR::enableBloom               = $HDRPostFX::enableBloom;
   $PostFXManager::Settings::HDR::enableBlueShift           = $HDRPostFX::enableBlueShift;
   $PostFXManager::Settings::HDR::enableToneMapping         = $HDRPostFX::enableToneMapping;
   $PostFXManager::Settings::HDR::gaussMean                 = $HDRPostFX::gaussMean;
   $PostFXManager::Settings::HDR::gaussMultiplier           = $HDRPostFX::gaussMultiplier;
   $PostFXManager::Settings::HDR::gaussStdDev               = $HDRPostFX::gaussStdDev;
   $PostFXManager::Settings::HDR::keyValue                  = $HDRPostFX::keyValue;
   $PostFXManager::Settings::HDR::minLuminace               = $HDRPostFX::minLuminace;
   $PostFXManager::Settings::HDR::whiteCutoff               = $HDRPostFX::whiteCutoff;
   $PostFXManager::Settings::ColorCorrectionRamp            = $HDRPostFX::colorCorrectionRamp;
   
   postVerbose("% - PostFX Manager - Settings Saved - HDR");      
}

function PostFXManager::settingsApplyAll(%this, %sFrom)
{
   // Apply settings which control if effects are on/off altogether.
   $PostFXManager::Settings::EnablePostFX        = $PostFXManager::PostFX::Enabled;  
   $PostFXManager::Settings::EnableHDR           = $PostFXManager::PostFX::EnableHDR;
      
   // Apply settings should save the values in the system to the 
   // the preset structure ($PostFXManager::Settings::*)


   // HDR settings
   %this.settingsApplyHDR();

   
   postVerbose("% - PostFX Manager - All Settings applied to $PostFXManager::Settings");
}

function PostFXManager::settingsApplyDefaultPreset(%this)
{
   PostFXManager::loadPresetHandler($PostFXManager::defaultPreset);
}

