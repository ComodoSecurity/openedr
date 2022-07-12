using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

using Be.Windows.Forms;

namespace Be.HexEditor
{
	/// <summary>
	/// Summary description for FormFindCancel.
	/// </summary>
	public class FormFindCancel : System.Windows.Forms.Form
	{
		HexBox _hexBox;

		private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.Label lblFinding;
		private System.Windows.Forms.Timer timer;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label lblPercent;
		private System.Windows.Forms.Timer timerPercent;
		private System.ComponentModel.IContainer components;

		public FormFindCancel()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FormFindCancel));
            this.btnCancel = new System.Windows.Forms.Button();
            this.lblFinding = new System.Windows.Forms.Label();
            this.timer = new System.Windows.Forms.Timer(this.components);
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label1 = new System.Windows.Forms.Label();
            this.lblPercent = new System.Windows.Forms.Label();
            this.timerPercent = new System.Windows.Forms.Timer(this.components);
            this.SuspendLayout();
            // 
            // btnCancel
            // 
            this.btnCancel.AccessibleDescription = null;
            this.btnCancel.AccessibleName = null;
            resources.ApplyResources(this.btnCancel, "btnCancel");
            this.btnCancel.BackgroundImage = null;
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Font = null;
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // lblFinding
            // 
            this.lblFinding.AccessibleDescription = null;
            this.lblFinding.AccessibleName = null;
            resources.ApplyResources(this.lblFinding, "lblFinding");
            this.lblFinding.ForeColor = System.Drawing.Color.Blue;
            this.lblFinding.Name = "lblFinding";
            // 
            // timer
            // 
            this.timer.Interval = 50;
            this.timer.Tick += new System.EventHandler(this.timer_Tick);
            // 
            // groupBox1
            // 
            this.groupBox1.AccessibleDescription = null;
            this.groupBox1.AccessibleName = null;
            resources.ApplyResources(this.groupBox1, "groupBox1");
            this.groupBox1.BackgroundImage = null;
            this.groupBox1.Font = null;
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.TabStop = false;
            // 
            // label1
            // 
            this.label1.AccessibleDescription = null;
            this.label1.AccessibleName = null;
            resources.ApplyResources(this.label1, "label1");
            this.label1.Font = null;
            this.label1.ForeColor = System.Drawing.Color.Blue;
            this.label1.Name = "label1";
            // 
            // lblPercent
            // 
            this.lblPercent.AccessibleDescription = null;
            this.lblPercent.AccessibleName = null;
            resources.ApplyResources(this.lblPercent, "lblPercent");
            this.lblPercent.Font = null;
            this.lblPercent.Name = "lblPercent";
            // 
            // timerPercent
            // 
            this.timerPercent.Tick += new System.EventHandler(this.timerPercent_Tick);
            // 
            // FormFindCancel
            // 
            this.AccessibleDescription = null;
            this.AccessibleName = null;
            resources.ApplyResources(this, "$this");
            this.BackColor = System.Drawing.SystemColors.Control;
            this.BackgroundImage = null;
            this.CancelButton = this.btnCancel;
            this.ControlBox = false;
            this.Controls.Add(this.lblPercent);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.lblFinding);
            this.Controls.Add(this.btnCancel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D;
            this.Icon = null;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "FormFindCancel";
            this.Deactivate += new System.EventHandler(this.FormFindCancel_Deactivate);
            this.Activated += new System.EventHandler(this.FormFindCancel_Activated);
            this.ResumeLayout(false);

		}
		#endregion

		public void SetHexBox(HexBox hexBox)
		{
			_hexBox = hexBox;
		}

		private void btnCancel_Click(object sender, System.EventArgs e)
		{
			DialogResult = DialogResult.Cancel;
			Close();
		}

		private void timer_Tick(object sender, System.EventArgs e)
		{
			if(lblFinding.Text.Length == 13)
				lblFinding.Text = "";

			lblFinding.Text += ".";
		}

		private void FormFindCancel_Activated(object sender, System.EventArgs e)
		{
			timer.Enabled = true;
			timerPercent.Enabled = true;
		}

		private void FormFindCancel_Deactivate(object sender, System.EventArgs e)
		{
			timer.Enabled = false;
		}

		private void timerPercent_Tick(object sender, System.EventArgs e)
		{
			long pos = _hexBox.CurrentFindingPosition;
			long length = _hexBox.ByteProvider.Length;
			double percent = (double)pos / (double)length * (double)100;
			
			System.Globalization.NumberFormatInfo nfi = 
				new System.Globalization.CultureInfo("en-US").NumberFormat;

			string text = percent.ToString("0.00", nfi) + " %";
			lblPercent.Text = text;
		}
	}
}
