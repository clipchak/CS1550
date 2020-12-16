/*
 * Colton Lipchak
 * CS1550 Operating Systems
 * Dr. Khattab
 * 15 November 2020
 * Project 3 - Second Chance Virtual Memory Simulator
 * 
 */

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.Scanner;

public class vmsim {
	//public variables to track accesses, faults, and writes
	public static int p1Accesses;
	public static int p1Faults;
	public static int p1Writes;

	public static int p2Accesses;
	public static int p2Faults;
	public static int p2Writes;
	
	//pointers to the two processes
	public static int p1Pointer;
	public static int p2Pointer;


	public static void main(String[] args) {
		//parse command line arguments
		int pageSize = Integer.parseInt(args[3]);
		int numFrames = Integer.parseInt(args[1]);
		String memSplitString = args[5];

		//calculate the number of bits to remove using the page size
		int bitsToRemove = (int) ( Math.log(pageSize * 1024) / Math.log(2) );
		
		//calculate the memory split and the frames between p1 and p2
		int p1Split = Integer.parseInt(memSplitString.charAt(0) + "");
		int p2Split = Integer.parseInt(memSplitString.charAt(memSplitString.length()-1) + "");
		int totalSplit = p1Split + p2Split;

		//calculate the number of frames
		int p1Frames = (int) (( (double)p1Split / (double)totalSplit) * numFrames);
		int p2Frames = (int) (( (double)p2Split / (double)totalSplit) * numFrames);
		
		//initialize the processes by setting them both to default values
		Page[] processOne = new Page[p1Frames];
		Page[] processTwo = new Page[p2Frames];
		
		for(int i = 0; i<processOne.length;i++) {
			processOne[i] = new Page();
		}
		
		for(int i = 0; i<processTwo.length;i++) {
			processTwo[i] = new Page();
		}
		
		//scan in the trace file line by line
		File file = new File(args[6]);
		Scanner in = null;
		
		try {
			
			in = new Scanner(new FileReader(file));
			
			//move line by line in the file
			while(in.hasNext()) {
				
				//get the current line of the file
				String line = in.nextLine();
				
				//get the mode (store 's' or load 'l')
				char mode = line.charAt(0);
				
				//get the address
				int endOfAddressIndex = line.indexOf(" ", line.indexOf(" ") + 1);
				long address = Long.parseUnsignedLong(line.substring(4, endOfAddressIndex),16);
				
				//shift the address
				address = address >> bitsToRemove;
		
				//find which process it belongs to
				int process = Integer.parseInt(line.substring(endOfAddressIndex + 1,line.length()));
				
				//create a new Page(mode, address, secondChance) 
				//with its secondChance initialized to false
				Page page = new Page(mode, address, false);
				
				//add the page to a certain process queue
				if(process == 0) {
					iterateP1(page,processOne);
				
				} else if(process == 1) {
					iterateP2(page,processTwo);
					
				} else { //error case
					System.out.print("A Process Number Error occurred while reading the file");
					return;
				}
				
			}
			
		} catch (FileNotFoundException e1) { //file not found
			System.out.println("File not found or does not exist");
		} finally { //close the file
			in.close();
		}
		
		//print statistics
		System.out.println("Algorithm: Second Chance");
		System.out.println("Number of frames: " + numFrames);
		System.out.println("Page size: " + args[3] + " KB");
		System.out.println("Total memory accesses: " + (p1Accesses + p2Accesses));
		System.out.println("Total page faults: " + (p1Faults + p2Faults));
		System.out.println("Total writes to disk: " + (p1Writes + p2Writes));

	}
	
	public static void iterateP1(Page page, Page[] process) {
		//increment the number of p1's accesses
		p1Accesses++;
		
		//find if the page exists and update it
		if(findAndUpdatePage(page,process) == false) {
			p1Pointer = replaceAndUpdateP1(page,process);
			p1Faults++;
			
		}
		
	}
	
	public static void iterateP2(Page page, Page[] process) {
		//increment the number of p2's accesses
		p2Accesses++;
		
		if(findAndUpdatePage(page,process) == false) {
			p2Pointer = replaceAndUpdateP2(page,process);
			p2Faults++;
			
		}
		
	}
	
	
    //find if the page exists in the queue
	//update the page to deserving of a second chance
	//and whether or not it is dirty
    public static boolean findAndUpdatePage(Page p, Page[] process) { 

        for(int i = 0; i < process.length; i++) { 
              
            if(process[i].getAddress() == p.getAddress()) { 
                
            	//set the page as getting a second chance
            	process[i].setSecondChance(true);
            	
            	//if the new page were loading in is dirty
            	//and the existing page is not already marked as dirty
            	//set the existing page to be dirty
            	if(p.getMode() == 's' && process[i].getMode() != 's') {
            		process[i].setMode('s');
            	}
                  
                //return true because there was a hit  
                //and a page does not need replaced
                return true; 
            } 
        }
          
        //return false  because the page doesn't exist s
        //and now a page to be replaced will be selected 
        return false; 
    } 
	
	
    public static int replaceAndUpdateP1(Page p, Page[] process) { 
        while(true) { 
              
            //found a page to replace that doesn't have a second chance 
        	if(process[p1Pointer].getSecondChance() == false) {
        		
        		//we write to disk if the page was dirty
        		if(process[p1Pointer].getMode() == 's') {
                	p1Writes++;
                }
        		
                //replace the old page with the new page 
                process[p1Pointer] = p; 
                
                //return updated pointer 
                return (p1Pointer+1)%process.length; 
            }
              
            //set the second chance to false so the page
        	// can now be replaced 
        	process[p1Pointer].setSecondChance(false);
              
            //update the pointer round robin style
            p1Pointer = (p1Pointer+1)%process.length; 
        }
    }
    
    public static int replaceAndUpdateP2(Page p, Page[] process) { 
        while(true) { 
              
            //found a page to replace that doesn't have a second chance 
        	if(process[p2Pointer].getSecondChance() == false) {
        		
        		//we write to disk if the page was dirty
        		if(process[p2Pointer].getMode() == 's') {
                	p2Writes++;
                }
        		
                //replace the old page with the new page 
                process[p2Pointer] = p; 
                
                //return updated pointer 
                return (p2Pointer+1)%process.length; 
            }
              
        	 //set the second chance to false so the page
        	// can now be replaced
        	process[p2Pointer].setSecondChance(false);
              
            //update the pointer round robin style
            p2Pointer = (p2Pointer+1)%process.length; 
        }
    }

}

