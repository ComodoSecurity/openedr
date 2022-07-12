object MainForm: TMainForm
  Left = 293
  Top = 286
  BorderStyle = bsDialog
  Caption = 'Traffic Shaper'
  ClientHeight = 139
  ClientWidth = 312
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 24
    Top = 64
    Width = 70
    Height = 13
    Caption = 'Process name:'
  end
  object Label2: TLabel
    Left = 10
    Top = 96
    Width = 85
    Height = 13
    Caption = 'Bytes per second:'
  end
  object Start: TButton
    Left = 16
    Top = 8
    Width = 88
    Height = 25
    Caption = 'Start'
    TabOrder = 0
    OnClick = StartClick
  end
  object Stop: TButton
    Left = 112
    Top = 8
    Width = 88
    Height = 25
    Caption = 'Stop'
    Enabled = False
    TabOrder = 1
    OnClick = StopClick
  end
  object ProcessName: TEdit
    Left = 101
    Top = 61
    Width = 196
    Height = 21
    TabOrder = 2
    Text = 'firefox.exe'
  end
  object Limit: TEdit
    Left = 101
    Top = 93
    Width = 196
    Height = 21
    TabOrder = 3
    Text = '10000'
  end
  object Timer1: TTimer
    Enabled = False
    OnTimer = Timer1Timer
    Left = 224
    Top = 32
  end
end
