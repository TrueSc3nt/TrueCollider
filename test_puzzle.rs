fn puzzle_range(puzzle_num: u32) -> (u128, u128) {
    let start = 1u128 << (puzzle_num - 1);
    let end = (1u128 << puzzle_num) - 1;
    (start, end)
}

fn stripped_range(puzzle_num: u32, zero_bytes: u32) -> (u128, u128) {
    let remaining_bits = puzzle_num - (zero_bytes * 8);
    let start = 1u128 << (remaining_bits - 1);
    let end = 1u128 << remaining_bits;
    (start, end)
}

fn main() {
    println!("Puzzle | ZeroBytes | Full Range         | Stripped Range     | Reduction");
    println!("-------|-----------|--------------------|--------------------|----------");
    for puzzle in [71u32, 72, 73, 74, 76, 80, 90, 100, 120, 135] {
        let zero_bytes = (256 - puzzle) / 8;
        let (fs, fe) = puzzle_range(puzzle);
        let full = fe - fs;
        let remaining = puzzle - (zero_bytes * 8);
        if remaining > 0 {
            let (ss, se) = stripped_range(puzzle, zero_bytes);
            let stripped = se - ss;
            let reduction = full as f64 / stripped as f64;
            println!("  {:4} | {:9} | {:>18} | {:>18} | {:>8.1}x", 
                puzzle, zero_bytes, full, stripped, reduction);
        }
    }
}
