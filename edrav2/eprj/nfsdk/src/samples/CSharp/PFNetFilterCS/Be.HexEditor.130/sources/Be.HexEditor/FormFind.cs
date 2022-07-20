using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

using Be.Windows.Forms;

namespace Be.HexEditor
{
	/// <summary>
	/// Summary description for FormFind.
	/// </summary>
	public class FormFind : System.Windows.Forms.Form
	{
		private Be.Windows.Forms.HexBox hexBox;
		private System.Windows.Forms.TextBox txtString;
		private System.Windows.Forms.RadioButton rbString;
		private System.Windows.Forms.RadioButton rbHex;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Button btnOK;
		private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.GroupBox groupBox1;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public FormFind()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
			rbString.CheckedChanged += new EventHandler(rb_CheckedChanged);
			rbHex.CheckedChanged += new EventHandler(rb_CheckedChanged);

//			rbString.Enter += new EventHandler(rbString_Enter);
//			rbHex.Enter += new EventHandler(rbHex_Enter);

			hexBox.ByteProvider = new DynamicByteProvider(new ByteCollection());
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FormFind));
            this.hexBox = new Be.Windows.Forms.HexBox();
            this.txtString = new System.Windows.Forms.TextBox();
            this.rbString = new System.Windows.Forms.RadioButton();
            this.rbHex = new System.Windows.Forms.RadioButton();
            this.label1 = new System.Windows.Forms.Label();
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.SuspendLayout();
            // 
            // hexBox
            // 
            resources.ApplyResources(this.hexBox, "hexBox");
            this.hexBox.LineInfoForeColor = System.Drawing.Color.Empty;
            this.hexBox.Name = "hexBox";
            this.hexBox.ShadowSelectionColor = System.Drawing.Color.FromArgb(((int)(((byte)(100)))), ((int)(((byte)(60)))), ((int)(((byte)(188)))), ((int)(((byte)(255)))));
            // 
            // txtString
            // 
            resources.ApplyResources(this.txtString, "txtString");
            this.txtString.Name = "txtString";
            // 
            // rbString
            // 
            this.rbString.Checked = true;
            resources.ApplyResources(this.rbString, "rbString");
            this.rbString.Name = "rbString";
            this.rbString.TabStop = true;
            // 
            // rbHex
            // 
            resources.ApplyResources(this.rbHex, "rbHex");
            this.rbHex.Name = "rbHex";
            // 
            // label1
            // 
            this.label1.ForeColor = System.Drawing.Color.Blue;
            resources.ApplyResources(this.label1, "label1");
            this.label1.Name = "label1";
            // 
            // btnOK
            // 
            resources.ApplyResources(this.btnOK, "btnOK");
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnOK.Name = "btnOK";
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            resources.ApplyResources(this.btnCancel, "btnCancel");
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // groupBox1
            // 
            resources.ApplyResources(this.groupBox1, "groupBox1");
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.TabStop = false;
            // 
            // FormFind
            // 
            this.AcceptButton = this.btnOK;
            resources.ApplyResources(this, "$this");
            this.BackColor = System.Drawing.SystemColors.Control;
            this.CancelButton = this.btnCancel;
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.rbHex);
            this.Controls.Add(this.rbString);
            this.Controls.Add(this.txtString);
            this.Controls.Add(this.hexBox);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "FormFind";
            this.Activated += new System.EventHandler(this.FormFind_Activated);
            this.ResumeLayout(false);
            this.PerformLayout();

		}
		#endregion

		public byte[] GetFindBytes()
		{
			if(rbString.Checked)
			{
				byte[] res = System.Text.ASCIIEncoding.ASCII.GetBytes(txtString.Text);
				return res;
			}
			else
			{
				return ((DynamicByteProvider)hexBox.ByteProvider).Bytes.GetBytes();
			}
		}

		private void rb_CheckedChanged(object sender, System.EventArgs e)
		{
			txtString.Enabled = rbString.Checked;
			hexBox.Enabled = !txtString.Enabled;

			if(txtString.Enabled)
				txtString.Focus();
			else
				hexBox.Focus();
		}

		private void rbString_Enter(object sender, EventArgs e)
		{
			txtString.Focus();
		}

		private void rbHex_Enter(object sender, EventArgs e)
		{
			hexBox.Focus();
		}

		private void FormFind_Activated(object sender, System.EventArgs e)
		{
			if(rbString.Checked)
				txtString.Focus();
			else
				hexBox.Focus();
		}

		private void btnOK_Click(object sender, System.EventArgs e)
		{
			if(rbString.Checked && txtString.Text.Length == 0)
				DialogResult = DialogResult.Cancel;
			else if(rbHex.Checked && hexBox.ByteProvider.Length == 0)
				DialogResult = DialogResult.Cancel;
			else
				DialogResult = DialogResult.OK;
		}

		private void btnCancel_Click(object sender, System.EventArgs e)
		{
			DialogResult = DialogResult.Cancel;
		}
	}
}
