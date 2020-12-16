
public class Page {
	private char mode;
	private long address;
	private boolean secondChance;
	
	//default page
	public Page() {
		mode = 0;
		address = -1;
		secondChance = false;
	}

	public Page(char mode, long address, boolean secondChance) {
		this.mode = mode;
		this.address = address;
		this.secondChance = secondChance;
	}
	
	public char getMode() {
		return mode;
	}
	
	public long getAddress() {
		return address;
	}
	
	public boolean getSecondChance() {
		return secondChance;
	}
	
	public void setSecondChance(boolean secondChance) {
		this.secondChance = secondChance;
	}
	
	public void setMode(char mode) {
		this.mode = mode;
	}
	
	public void setAddress(long address) {
		this.address = address;
	}

}
