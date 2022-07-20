object MainForm: TMainForm
  Left = 293
  Top = 286
  Width = 718
  Height = 630
  Caption = 'NetRedirector'
  Color = clBtnFace
  Constraints.MinHeight = 500
  Constraints.MinWidth = 718
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  DesignSize = (
    702
    592)
  PixelsPerInch = 96
  TextHeight = 13
  object Label5: TLabel
    Left = 234
    Top = 14
    Width = 108
    Height = 13
    Caption = 'Local proxy process id:'
  end
  object GroupBox1: TGroupBox
    Left = 8
    Top = 39
    Width = 358
    Height = 129
    Caption = 'TCP protocol'
    TabOrder = 0
    object Label1: TLabel
      Left = 10
      Top = 54
      Width = 22
      Height = 13
      Caption = 'Port:'
    end
    object Label2: TLabel
      Left = 10
      Top = 78
      Width = 256
      Height = 13
      Caption = 'Remote proxy (x.x.x.x:port for IPv4, [x..x]:port for IPv6):'
    end
    object PortEdit: TEdit
      Left = 39
      Top = 50
      Width = 81
      Height = 21
      TabOrder = 0
      Text = '110'
    end
    object ProxyAddressEdit: TEdit
      Left = 10
      Top = 95
      Width = 217
      Height = 21
      TabOrder = 1
      Text = '163.15.64.8:3128'
    end
    object RedirectTCP: TCheckBox
      Left = 10
      Top = 22
      Width = 336
      Height = 17
      Caption = 'Redirect outgoing TCP connections to the specified proxy.'
      Checked = True
      State = cbChecked
      TabOrder = 2
    end
    object ProxyTypeCombo: TComboBox
      Left = 232
      Top = 95
      Width = 113
      Height = 21
      Style = csDropDownList
      ItemHeight = 13
      ItemIndex = 1
      TabOrder = 3
      Text = 'HTTPS proxy'
      Items.Strings = (
        'HTTP proxy'
        'HTTPS proxy'
        'SOCKS proxy')
    end
  end
  object Start: TButton
    Left = 8
    Top = 8
    Width = 88
    Height = 25
    Caption = 'Start'
    TabOrder = 1
    OnClick = StartClick
  end
  object Stop: TButton
    Left = 104
    Top = 8
    Width = 88
    Height = 25
    Caption = 'Stop'
    Enabled = False
    TabOrder = 2
    OnClick = StopClick
  end
  object GroupBox2: TGroupBox
    Left = 368
    Top = 39
    Width = 333
    Height = 129
    Caption = 'UDP protocol'
    TabOrder = 3
    object Label4: TLabel
      Left = 10
      Top = 78
      Width = 246
      Height = 13
      Caption = 'DNS server (x.x.x.x:port for IPv4, [x..x]:port for IPv6):'
    end
    object Label3: TLabel
      Left = 29
      Top = 36
      Width = 95
      Height = 13
      Caption = 'to the specified host'
    end
    object DNSServerAddressEdit: TEdit
      Left = 10
      Top = 95
      Width = 217
      Height = 21
      TabOrder = 0
      Text = '208.67.222.222:53'
    end
    object RedirectUDP: TCheckBox
      Left = 10
      Top = 20
      Width = 263
      Height = 19
      Caption = 'Redirect outgoing DNS requests (UDP, port 53) '
      Checked = True
      State = cbChecked
      TabOrder = 1
    end
  end
  object GroupBox3: TGroupBox
    Left = 8
    Top = 169
    Width = 694
    Height = 420
    Anchors = [akLeft, akTop, akRight, akBottom]
    Caption = 'Filtering log'
    TabOrder = 4
    object Log: TMemo
      Left = 2
      Top = 15
      Width = 690
      Height = 403
      Align = alClient
      ScrollBars = ssBoth
      TabOrder = 0
    end
  end
  object ProxyPidEdit: TEdit
    Left = 355
    Top = 10
    Width = 78
    Height = 21
    TabOrder = 5
    Text = '0'
  end
end
